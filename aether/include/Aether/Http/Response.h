// Response.h
#ifndef AETHER_HTTP_RESPONSE_H
#define AETHER_HTTP_RESPONSE_H

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <algorithm>
#include "Aether/Core/json.hpp"

namespace Aether {
namespace Http {

struct Response {
    void send(const std::string& body, int statusCode = 200) {
        this->body = body;
        this->statusCode = statusCode;
    }

    void sendJSON(const std::string& json, int statusCode = -1) {
        setHeader("Content-Type", "application/json");
        
        // If no status code provided, use the current one
        if (statusCode == -1) {
            statusCode = this->statusCode;
        }
        
        send(json, statusCode);
    }

    void setHeader(const std::string& key, const std::string& value) {
        headers[key] = value;
    }

    void setStatus(int statusCode) {
        this->statusCode = statusCode;
    }

    void render(const std::string& viewName, const nlohmann::json& data = {}) {
        std::error_code ec;
        auto base = std::filesystem::weakly_canonical(viewsFolder_, ec);
        if (ec) {
            send("404 Not Found", 404);
            return;
        }

        auto candidate = std::filesystem::weakly_canonical(base / (viewName + ".html"), ec);
        const bool underBase = !ec && std::mismatch(
            base.begin(), base.end(), candidate.begin(), candidate.end()
        ).first == base.end();

        if (ec || !underBase) {
            send("404 Not Found", 404);
            return;
        }

        std::ifstream file(candidate);
        if (!file.is_open()) {
            send("404 Not Found", 404);
            return;
        }
        std::ostringstream buffer;
        buffer << file.rdbuf();
        std::string html = buffer.str();
        // 1. Handle {{#if key}} ... {{else}} ... {{/if}}
        {
            std::regex if_regex(R"(\{\{#if ([a-zA-Z0-9_\.]+)\}\}([\s\S]*?)(?:\{\{else\}\}([\s\S]*?))?\{\{\/if\}\})");
            std::smatch m;
            std::string processed;
            std::string::const_iterator searchStart(html.cbegin());
            while (std::regex_search(searchStart, html.cend(), m, if_regex)) {
                processed.append(searchStart, m[0].first);
                std::string key = m[1].str();
                std::string if_block = m[2].str();
                std::string else_block = m.size() > 3 ? m[3].str() : "";
                nlohmann::json val = data;
                size_t pos = 0;
                std::string keyCopy = key;
                while ((pos = keyCopy.find('.')) != std::string::npos) {
                    val = val[keyCopy.substr(0, pos)];
                    keyCopy = keyCopy.substr(pos + 1);
                }
                bool cond = false;
                if (val.contains(keyCopy)) {
                    if (val[keyCopy].is_boolean()) cond = val[keyCopy].get<bool>();
                    else cond = !val[keyCopy].empty();
                }
                processed += cond ? if_block : else_block;
                searchStart = m[0].second;
            }
            processed.append(searchStart, html.cend());
            html = processed;
        }
        // 2. Handle {{#each key}} ... {{/each}}
        {
            std::regex each_regex(R"(\{\{#each ([a-zA-Z0-9_\.]+)\}\}([\s\S]*?)\{\{\/each\}\})");
            std::smatch m;
            std::string processed;
            std::string::const_iterator searchStart(html.cbegin());
            while (std::regex_search(searchStart, html.cend(), m, each_regex)) {
                processed.append(searchStart, m[0].first);
                std::string key = m[1].str();
                std::string block = m[2].str();
                nlohmann::json val = data;
                size_t pos = 0;
                std::string keyCopy = key;
                while ((pos = keyCopy.find('.')) != std::string::npos) {
                    val = val[keyCopy.substr(0, pos)];
                    keyCopy = keyCopy.substr(pos + 1);
                }
                std::string result;
                if (val.contains(keyCopy) && val[keyCopy].is_array()) {
                    for (const auto& item : val[keyCopy]) {
                        std::string item_block = block;
                        // Replace {{this}} with item value (string or dump)
                        std::regex this_regex(R"(\{\{this\}\})");
                        if (item.is_string()) {
                            item_block = std::regex_replace(item_block, this_regex, htmlEscape(item.get<std::string>()));
                        } else if (item.is_object()) {
                            // Replace {{key}} inside block with item[key]
                            std::regex inner_var_regex(R"(\{\{([a-zA-Z0-9_]+)\}\})");
                            std::smatch m2;
                            std::string item_processed;
                            std::string::const_iterator item_search(item_block.cbegin());
                            while (std::regex_search(item_search, item_block.cend(), m2, inner_var_regex)) {
                                item_processed.append(item_search, m2[0].first);
                                std::string k = m2[1].str();
                                if (item.contains(k))
                                    item_processed += item[k].is_string() ? htmlEscape(item[k].get<std::string>()) : htmlEscape(item[k].dump());
                                item_search = m2[0].second;
                            }
                            item_processed.append(item_search, item_block.cend());
                            item_block = item_processed;
                        } else {
                            item_block = std::regex_replace(item_block, this_regex, htmlEscape(item.dump()));
                        }
                        result += item_block;
                    }
                }
                processed += result;
                searchStart = m[0].second;
            }
            processed.append(searchStart, html.cend());
            html = processed;
        }

        // 3. Replace {{key}} with value from data (after blocks)
        {
            std::regex var_regex(R"(\{\{([a-zA-Z0-9_\.]+)\}\})");
            std::smatch m;
            std::string processed;
            std::string::const_iterator searchStart(html.cbegin());
            while (std::regex_search(searchStart, html.cend(), m, var_regex)) {
                processed.append(searchStart, m[0].first);
                std::string key = m[1].str();
                nlohmann::json val = data;
                size_t pos = 0;
                std::string keyCopy = key;
                while ((pos = keyCopy.find('.')) != std::string::npos) {
                    val = val[keyCopy.substr(0, pos)];
                    keyCopy = keyCopy.substr(pos + 1);
                }
                if (val.contains(keyCopy))
                    processed += val[keyCopy].is_string() ? htmlEscape(val[keyCopy].get<std::string>()) : htmlEscape(val[keyCopy].dump());
                searchStart = m[0].second;
            }
            processed.append(searchStart, html.cend());
            html = processed;
        }

        send(html);
    }

    void sendFile(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary);
        if (file.is_open()) {
            std::ostringstream buffer;
            buffer << file.rdbuf();
            send(buffer.str());
        } else {
            send("404 - File Not Found", 404);
        }
    }

    std::string body;
    int statusCode = 200; // Default to 200 OK
    std::unordered_map<std::string, std::string> headers;

    // Declare the static member variable
    static std::string viewsFolder_;

private:
    static std::string htmlEscape(const std::string& value) {
        std::string escaped;
        escaped.reserve(value.size());
        for (char c : value) {
            switch (c) {
                case '&': escaped += "&amp;"; break;
                case '<': escaped += "&lt;"; break;
                case '>': escaped += "&gt;"; break;
                case '"': escaped += "&quot;"; break;
                case '\'': escaped += "&#x27;"; break;
                case '/': escaped += "&#x2F;"; break;
                default: escaped += c; break;
            }
        }
        return escaped;
    }
};

} // namespace Http
} // namespace Aether

#endif // Aether_HTTP_RESPONSE_H
