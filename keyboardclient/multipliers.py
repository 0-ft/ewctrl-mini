import csv
import numpy as np

def load_groups_and_scales(filename):
    groups = {}
    scales = {}
    with open(filename, mode='r') as file:
        reader = csv.DictReader(file)
        for row in reader:
            if(row["Group"] and row["Multiplier"]):
                    group = row["Group"]
                    multiplier = [int(x) for x in row["Multiplier"].split(",")]
                    groups[group] = np.array(multiplier)
                    for col_name in row:
                        if col_name.startswith("Color:"):
                            color_name = col_name.split(":")[1]
                            scales.setdefault(color_name, {})[group] = float(row[col_name]) / 255 if row[col_name] else 0

    return (groups, scales)

def make_sum_multiplier(groups, group_scales):
    result = np.zeros(len(next(iter(groups.values()))))
    for group_name, group_multiplier in groups.items():
        if(group_name in group_scales):
            result += groups[group_name] * group_scales[group_name]
    result = np.clip(result, 0, 1)
    return result

def load_multipliers(filename):
    groups, scales = load_groups_and_scales("data/channel_map.csv")
    multipliers = {
        color_name: (make_sum_multiplier(groups, group_scales) * 4095).astype(int).tolist()
        for color_name, group_scales in scales.items()
    }
    return multipliers

# print(load_multipliers("data/channel_map.csv"))