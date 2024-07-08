import gzip
from sys import argv
import numpy as np
from lxml import etree as ET
import bezier
# import seaborn
from matplotlib import pyplot as plt

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


# load automation lanes from an ableton als file
def parse_automations(filepath):

    with gzip.open(filepath, "r") as xml:
        open("als.xml", "wb").write(xml.read())

    with gzip.open(filepath, "r") as xml:
        tree = ET.parse(xml)

    root = tree.getroot()

    pointee_envelopes = {}
    envelopes = list(root.iter("AutomationEnvelope"))
    pointee_envelopes = {
        int(envelope.find("EnvelopeTarget").find("PointeeId").get("Value")): [e.attrib for e in envelope.find("Automation").find("Events").findall("*")]
        for envelope in envelopes
        if envelope is not None
    }



    # print(pointee_envelopes)

    float_params = list(root.iter("MxDFloatParameter"))
    param_to_pointee = lambda p: p.find(".//AutomationTarget").get("Id")

    
    # pointee_params = {
    #     param_to_pointee(param): param
    #     for param in float_params
    #     if param.find(".//AutomationTarget") is not None
    # }
    # print(pointee_params)

    # target_map = {
    #     t.find(".//AutomationTarget").get("Id"): t.find("Name").get("Value")
    #     for t in float_params
    #     if t.find(".//AutomationTarget") is not None
    # }

    # # lomid_map = {
    # #     t.find("Name").get("Value"): t.find(".//LomId").get("Value")
    # #     for t in float_params
    # #     if t.find(".//AutomationTarget")
    # # }

    # # print(lomid_map)

    group_devices = list(root.iter("AudioEffectGroupDevice"))

    for device in group_devices:
        print(create_macro_display_map(device))
    #     # print(ET.tostring(device))
    #     # params = list(device.iter("MxDFloatParameter"))
    #     # print(params)
    #     # print(ET.tostring(params[0]))
    #     lomid_to_id = parse_lom_id_to_mxdidref(device)
    #     print(lomid_to_id)
    #     # parameters = {int(p.get("Id")): p.find(".//AutomationTarget").get("Id") for p in device.iter("MxDFloatParameter")}
    #     # print(parameters)
    #     # disp_names = list(device.xpath("*[starts-with(name(), 'MacroDisplayNames.')]"))
    #     # print(disp_names)
    #     # name_numbers = {
    #     #     int(disp_name.tag.split(".")[-1]): disp_name.get("Value")
    #     #     for disp_name in disp_names
    #     # }
    #     # print(name_numbers)
    #     # name_targets = {
    #     #     name: parameters[id]
    #     #     for id, name in name_numbers.items()
    #     # }
    #     # print(name_targets)
    #     # exit()
    # # midi_tracks = root.iter("MidiTrack")
    # # envelopes = list(root.iter("AutomationEnvelope"))


    # return automations

# parse a single segment between two points a and b of an ableton automation track, create a bezier curve object
def curve_between(a, b):
    ax, bx = a["Time"], b["Time"]
    ay, by = a["Value"], b["Value"]

    if "CurveControl1X" not in a:
        nodes = np.asfortranarray([
            [ax, bx],
            [ay, by]
        ])
        return bezier.Curve(nodes, degree=1)

    dx, dy = bx - ax, by - ay

    c1x = ax + a["CurveControl1X"] * dx
    c1y = ay + a["CurveControl1Y"] * dy

    c2x = ax + a["CurveControl2X"] * dx
    c2y = ay + a["CurveControl2Y"] * dy

    nodes = np.asfortranarray([
        [ax, c1x, c2x, bx],
        [ay, c1y, c2y, by]
    ])
    print(nodes)
    curve = bezier.Curve(nodes, degree=3)
    return curve

# parse an entire ableton automation track into a list of (start_time, curve) tuples
def parse_automation(events):
    events = [{k: float(v) for k, v in e.items()} for e in events]
    points = [e for e in events if e["Time"] >= 0]
    segments = [(a["Time"], b["Time"], curve_between(a, b)) for a, b in zip(points, points[1:])]
    return segments

# generate samples of a complete automation track at given times
def sample_automation(segments, times):
    values = []
    segi = 0
    end_time = segments[-1][1]
    for t in times:
        assert t <= end_time, f"time {t} out of range {segments[0][0]}-{end_time}"
        while segi < (len(segments) - 1) and segments[segi][1] <= t:
            segi += 1
        seg_length = segments[segi][1] - segments[segi][0]
        offset = t - segments[segi][0]
        f = offset / seg_length
        v = segments[segi][2].evaluate(f)[1, 0]
        values.append((t, v))
    return np.array(values)

automations = parse_automations(argv[1])

print(automations)
res = parse_automation(automations["C1L0"])
print(res)
# print(res)
samples = sample_automation(res, np.linspace(60, 62.0, 300))
# print(samples)
# plt.plot(samples[:, 0], samples[:, 1])
# plt.show()