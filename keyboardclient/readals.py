import gzip
import json
import re
from sys import argv
import numpy as np
from lxml import etree as ET

def load_als(filepath):
    with gzip.open(filepath, "r") as xml:
        open("als.xml", "wb").write(xml.read())

    with gzip.open(filepath, "r") as xml:
        tree = ET.parse(xml)

    root = tree.getroot()
    return root

def find_locators(root):
    locator_elements = root.findall('.//Locator')
    locators = []
    for locator in locator_elements:
        locator_id = locator.get('Id')
        locator_name = locator.find('.//Name').get('Value')
        locator_time = float(locator.find('.//Time').get('Value'))
        locators.append((locator_name, locator_time))
    return sorted(locators, key=lambda x: x[1])

def extract_macro_mappings(root):
    # Initialize the dictionary to store the mappings
    macro_mappings = {}

    # Define the regex pattern to match the tag name MacroControls.([0-9]+)$
    pattern = re.compile(r'^MacroControls\.(\d+)$')

    # Iterate through all elements in the root
    for element in root.iter():
        # Check if the tag name matches the pattern
        match = pattern.match(element.tag)
        if match:
            # Extract the macro number from the tag name
            macro_number = match.group(1)

            # Find the sibling with the tag name MacroDisplayNames.<macro_number>
            display_name_tag = f'MacroDisplayNames.{macro_number}'
            display_name_element = element.getparent().find(display_name_tag)
            if display_name_element is not None:
                display_name = display_name_element.get('Value')
                
                # Find the descendant with the tag name AutomationTarget
                automation_target_element = element.find('AutomationTarget')
                if automation_target_element is not None:
                    pointee_id = int(automation_target_element.get('Id'))
                    
                    # Add the mapping to the dictionary
                    if display_name and pointee_id:
                        macro_mappings[display_name] = pointee_id

    return macro_mappings

def read_envelopes(root):
    pointee_envelopes = {}
    envelopes = list(root.iter("AutomationEnvelope"))
    pointee_envelopes = {
        int(envelope.find("EnvelopeTarget").find("PointeeId").get("Value")): [e.attrib for e in envelope.find("Automation").find("Events").findall("*")]
        for envelope in envelopes
        if envelope is not None
    }

    # convert all values to floats
    pointee_envelopes = {
        pointee: [{
            k: float(v) 
            for k, v in event.items() 
            if k in ["Time", "Value", "CurveControl1X", "CurveControl1Y", "CurveControl2X", "CurveControl2Y"]
            }
            for event in envelope
        ]
        for pointee, envelope in pointee_envelopes.items()
    }

    return pointee_envelopes

def cut_envelope(envelope, start_time, end_time):
    # print(f"cutting envelope {envelope} from {start_time} to {end_time}")
    cut = [
        (event | { "Time": event["Time"] - start_time})
        for event in envelope
        if event["Time"] >= start_time and event["Time"] <= end_time
    ]
    
    while len(cut) > 1 and cut[-2]["Time"] == cut[-1]["Time"]:
        cut.pop()
    return cut

def sanitise_envelope(envelope):
    envelope = [event for event in envelope if event["Time"] >= 0]
    
    rounding = 2
    envelope = [[
        event["Time"],
        round(event["Value"] / 127.0, rounding),
        round(event.get("CurveControl1X", 0), rounding),
        round(event.get("CurveControl1Y", 0), rounding),
        round(event.get("CurveControl2X", 0), rounding),
        round(event.get("CurveControl2Y", 0), rounding)
    ] if "CurveControl1X" in event else [
        event["Time"],
        round(event["Value"] / 127.0, rounding),
    ] for event in envelope
    ]

    return envelope

def patterns_size_info(patterns):
    all = [x for p in patterns for o in p["data"] for s in o for x in s]
    print(f"Numbers in all patterns: {len(all)}")

def generate_patterns(filepath):
    root = load_als(filepath)
    pointee_envelopes = read_envelopes(root)
    name_pointees = extract_macro_mappings(root)

    name_envelopes = {
        name: pointee_envelopes[pointee]
        for name, pointee in name_pointees.items()
        if pointee in pointee_envelopes
    }
    print(f"Found {len(name_envelopes)} envelopes: {list(name_envelopes.keys())}")

    locators = find_locators(root)
    print(f"Found {len(locators)} locators: {locators}")

    patterns = {}
    for start_locator, end_locator in zip(locators, locators[1:]):
        start_name = start_locator[0]
        start_time = start_locator[1]
        # end_name = end_locator[0]
        end_time = end_locator[1]
        print(f"Pattern {start_name} from {start_time} to {end_time}")
        patterns[start_name] = {
            envelope_name: cut_envelope(envelope, start_time, end_time)
            for envelope_name, envelope in name_envelopes.items()
        }

    # print(patterns)

    to_save = [
        {
            "name" : name,
            "data": [
                sanitise_envelope(envelope)
                for name, envelope in sorted(list(envelopes.items()), key=lambda x: x[0])
            ]
        }
        for name, envelopes in patterns.items()
    ]
    
    print(f"generated {len(to_save)} patterns")

    json_out = json.dumps(to_save, indent=2)
    open("patterns.json", "w").write(json_out)
    patterns_size_info(to_save)
    return to_save
    # exit()



if __name__ == "__main__":
    if len(argv) < 2:
        print("Usage: python readals.py <filepath>")
        exit()

    filepath = argv[1]
    print(generate_patterns(filepath))

# final = read_envelopes(root)
# locators = find_locators(root)
# # print(locators)

# patterns = {}
# for envelope_name, envelope in final.items():
#     splits = split_envelope(envelope, locators)
#     for locator, section in splits.items():
#         patterns.setdefault(locator, {})[envelope_name] = section

# final = {
#     name: sanitise_envelope(envelope)
#     for name, envelope in final.items()
# }

# open("patterns.json", "w").write(json.dumps([list(final.values())], indent=2))
