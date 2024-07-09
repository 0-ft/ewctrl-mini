import gzip
import json
from sys import argv
import numpy as np
from lxml import etree as ET
import bezier
# import seaborn
from matplotlib import pyplot as plt
# from json import encoder
# encoder.FLOAT_REPR = lambda o: format(o, '.3f')

# seaborn.set()

# def parse_lom_id_to_mxdidref(xml_element):
#     lom_id_map = {}
#     for mxdidref in xml_element.xpath('.//MxDIdRef'):
#         mxdidref_id = mxdidref.get('Id')
#         lom_id_value = mxdidref.find('.//LomId').get('Value')
#         lom_id_map[lom_id_value] = mxdidref_id
#     return lom_id_map

def create_macro_display_map(audio_effect_group_device):
    # Initialize the result dictionary
    macro_display_map = {}

    # Get all MacroDisplayNames elements
    macro_display_names = audio_effect_group_device.findall('.//MacroDisplayNames.*')
    
    # Get all MxDFloatParameter elements
    mxdfloat_parameters = audio_effect_group_device.findall('.//MxDFloatParameter')
    
    # Create the map
    for i, macro_display in enumerate(macro_display_names):
        display_name = macro_display.attrib['Value']
        # Assuming the index of MacroDisplayNames corresponds to the index of MxDFloatParameter
        if i < len(mxdfloat_parameters):
            automation_target = mxdfloat_parameters[i].find('.//AutomationTarget')
            if automation_target is not None:
                automation_id = automation_target.attrib['Id']
                macro_display_map[display_name] = int(automation_id)
    
    return macro_display_map

def load_als(filepath):

    with gzip.open(filepath, "r") as xml:
        open("als.xml", "wb").write(xml.read())

    with gzip.open(filepath, "r") as xml:
        tree = ET.parse(xml)

    root = tree.getroot()
    return root

def find_locators(root):
    locator_elements = root.findall('.//Locator')
    locators = {}
    for locator in locator_elements:
        locator_id = locator.get('Id')
        locator_name = locator.find('.//Name').get('Value')
        locator_time = float(locator.find('.//Time').get('Value')) * 135 * 2 / 60
        locators[locator_name] = locator_time
    return locators


# load automation lanes from an ableton als file
def parse_automations(root):

    pointee_envelopes = {}
    envelopes = list(root.iter("AutomationEnvelope"))
    pointee_envelopes = {
        int(envelope.find("EnvelopeTarget").find("PointeeId").get("Value")): [e.attrib for e in envelope.find("Automation").find("Events").findall("*")]
        for envelope in envelopes
        if envelope is not None
    }

    # convert all values to floats
    pointee_envelopes = {
        pointee: [{k: float(v) for k, v in event.items()} for event in envelope]
        for pointee, envelope in pointee_envelopes.items()
    }

    # print(pointee_envelopes)

    float_params = list(root.iter("MxDFloatParameter"))
    param_to_pointee = lambda p: int(p.find(".//AutomationTarget").get("Id"))
    param_to_name = lambda p: p.find("Name").get("Value")
    name_pointees = {
        param_to_name(param): param_to_pointee(param)
        for param in float_params
    }
    # print(name_pointees)
    name_envelopes = {
        name: pointee_envelopes[pointee]
        for name, pointee in name_pointees.items()
        if pointee in pointee_envelopes
    }
    return name_envelopes

#split envelope into multiple envelopes based on locators
def split_envelope(envelope, locators):
    splits = {}
    envelope = sorted(envelope, key=lambda event: event["Time"])
    for event in envelope:
        for start_locator, end_locator in zip(locators.items(), list(locators.items())[1:]):
            start_time = start_locator[1]
            start_name = start_locator[0]
            end_time = end_locator[1]
            if event["Time"] >= start_time and event["Time"] <= end_time:
                splits.setdefault(start_name, []).append(event)
    return splits

def sanitise_envelope(envelope):
    # remove points with negative time
    envelope = [event for event in envelope if event["Time"] >= 0]
    
    # subtract the time of the first event from all events
    start_time = envelope[0]["Time"]
    print(f"start time: {start_time}")
    envelope = [{
        "Time": event["Time"] - start_time,
        "Value": event["Value"],
        "CurveControl1X": event.get("CurveControl1X", 0),
        "CurveControl1Y": event.get("CurveControl1Y", 0),
        "CurveControl2X": event.get("CurveControl2X", 0),
        "CurveControl2Y": event.get("CurveControl2Y", 0)
    } if "CurveControl1X" in event else {
        "Time": event["Time"] - start_time,
        "Value": event["Value"]
    } for event in envelope]

    envelope = [[
        event["Time"],
        event["Value"],
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
final = parse_automations(root)
locators = find_locators(root)
print(locators)

patterns = {}
for envelope_name, envelope in final.items():
    splits = split_envelope(envelope, locators)
    for locator, section in splits.items():
        patterns.setdefault(locator, {})[envelope_name] = section
        # print(f"{name} {locator}")
        # print(sanitise_envelope(split_envelope))
        # print("")

# print(patterns)

final = {
    name: sanitise_envelope(envelope)
    for name, envelope in final.items()
}

open("patterns.json", "w").write(json.dumps([list(final.values())], indent=2))
# cpp = generate_cpp_bezier_patterns_header([[env for name, env in automations.items()]])
# open("include/BezierFaderPatterns.h", "w").write(cpp)
# automations_to_cpp(automations)
# print(automations)



# print(automations)
# res = parse_automation(automations["C1L0"])
# print(res)
# print(res)
# samples = sample_automation(res, np.linspace(60, 62.0, 300))
# print(samples)
# plt.plot(samples[:, 0], samples[:, 1])
# plt.show()
