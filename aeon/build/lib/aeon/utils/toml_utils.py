import toml

def read_toml(file_path):
    with open(file_path, "r") as file:
        return toml.load(file)

def write_toml(file_path, data):
    with open(file_path, "w") as file:
        toml.dump(data, file)