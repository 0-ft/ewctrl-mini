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




# def create_macro_display_map(audio_effect_group_device):
#     # Initialize the result dictionary
#     macro_display_map = {}

#     # Get all MacroDisplayNames elements
#     macro_display_names = audio_effect_group_device.findall('.//MacroDisplayNames.*')
    
#     # Get all MxDFloatParameter elements
#     mxdfloat_parameters = audio_effect_group_device.findall('.//MxDFloatParameter')
    
#     # Create the map
#     for i, macro_display in enumerate(macro_display_names):
#         display_name = macro_display.attrib['Value']
#         # Assuming the index of MacroDisplayNames corresponds to the index of MxDFloatParameter
#         if i < len(mxdfloat_parameters):
#             automation_target = mxdfloat_parameters[i].find('.//AutomationTarget')
#             if automation_target is not None:
#                 automation_id = automation_target.attrib['Id']
#                 macro_display_map[display_name] = int(automation_id)
    
#     return macro_display_map


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

# #split envelope into multiple envelopes based on locators
# def split_envelope(envelope, locators):
#     splits = {}
#     envelope = sorted(envelope, key=lambda event: event["Time"])
#     for event in envelope:
#         for start_locator, end_locator in zip(locators.items(), list(locators.items())[1:]):
#             start_time = start_locator[1]
#             start_name = start_locator[0]
#             end_time = end_locator[1]
#             if event["Time"] >= start_time and event["Time"] <= end_time:
#                 splits.setdefault(start_name, []).append(event)
#     return splits


def sanitise_envelope(envelope):
    # remove points with negative time
    envelope = [event for event in envelope if event["Time"] >= 0]
    
    # envelope = [{
    #     "Time": event["Time"],
    #     "Value": event["Value"],
    #     "CurveControl1X": event.get("CurveControl1X", 0),
    #     "CurveControl1Y": event.get("CurveControl1Y", 0),
    #     "CurveControl2X": event.get("CurveControl2X", 0),
    #     "CurveControl2Y": event.get("CurveControl2Y", 0)
    # } if "CurveControl1X" in event else {
    #     "Time": event["Time"],
    #     "Value": event["Value"]
    # } for event in envelope]

    envelope = [[
        event["Time"],
        round(event["Value"] / 127.0, 4),
        round(event.get("CurveControl1X", 0), 4),
        round(event.get("CurveControl1Y", 0), 4),
        round(event.get("CurveControl2X", 0), 4),
        round(event.get("CurveControl2Y", 0), 4)
    ] if "CurveControl1X" in event else [
        event["Time"],
        event["Value"],
    ] for event in envelope
    ]

    # envelope = [{
    #     "T": event["Time"] - start_time,
    #     "V": event["Value"],
    #     "C1X": round(event.get("CurveControl1X", 0), 4),
    #     "C1Y": round(event.get("CurveControl1Y", 0), 4),
    #     "C2X": round(event.get("CurveControl2X", 0), 4),
    #     "C2Y": round(event.get("CurveControl2Y", 0), 4)
    # } if "CurveControl1X" in event else {
    #     "T": event["Time"] - start_time,
    #     "V": event["Value"]
    # } for event in envelope]


    return envelope

root = load_als(argv[1])
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


to_save = [
    {
        "name" : name,
        "data": [
            sanitise_envelope(envelope)
            for envelope in envelopes.values()
        ]
    }
    for name, envelopes in patterns.items()
]

json_out = json.dumps(to_save[:10], indent=2)
open("patterns.json", "w").write(json_out)
exit()



final = read_envelopes(root)
locators = find_locators(root)
# print(locators)

patterns = {}
for envelope_name, envelope in final.items():
    splits = split_envelope(envelope, locators)
    for locator, section in splits.items():
        patterns.setdefault(locator, {})[envelope_name] = section

final = {
    name: sanitise_envelope(envelope)
    for name, envelope in final.items()
}

open("patterns.json", "w").write(json.dumps([list(final.values())], indent=2))
