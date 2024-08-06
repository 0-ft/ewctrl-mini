import gzip
import json
import re
from sys import argv
from lxml import etree as ET
import logging

def load_als(filepath):
    # with gzip.open(filepath, "r") as xml:
    #     open("als.xml", "wb").write(xml.read())

    with gzip.open(filepath, "r") as xml:
        tree = ET.parse(xml)

    root = tree.getroot()
    return root

def get_tempo(root: ET.Element):
    tempo = next(x for x in root.findall('.//Tempo') if x.get('Value'))
    return float(tempo.get('Value'))

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
    # logging.debug(f"Cutting envelope {envelope} from {start_time} to {end_time}")
    cut = [
        (event | { "Time": event["Time"] - start_time})
        for event in envelope
        if event["Time"] >= start_time and event["Time"] <= end_time
    ]
    
    while len(cut) > 1 and cut[-2]["Time"] == cut[-1]["Time"]:
        cut.pop()
    return cut

def remove_redundant_points(points):
    if all([p[1] == 0 for p in points]):
        return []

    if len(points) <= 2:
        return points  # No points to remove if there are 2 or fewer points

    result = [points[0]]  # Keep the first point
    for i in range(1, len(points) - 1):
        if points[i][1] != points[i-1][1] or points[i][1] != points[i+1][1]:
            result.append(points[i])

    result.append(points[-1])  # Keep the last point
    return result

def sanitise_envelope(envelope, tempo):
    envelope = [event for event in envelope if event["Time"] >= 0]
    
    rounding = 2
    envelope = [[
        round(event["Time"] * 60 / tempo, rounding),
        round(event["Value"] / 127.0, rounding),
        round(event.get("CurveControl1X", 0), rounding),
        round(event.get("CurveControl1Y", 0), rounding),
        round(event.get("CurveControl2X", 0), rounding),
        round(event.get("CurveControl2Y", 0), rounding)
    ] if "CurveControl1X" in event else [
        round(event["Time"] * 60 / tempo, rounding),
        round(event["Value"] / 127.0, rounding),
    ] for event in envelope
    ]

    envelope = [[int(x) if x.is_integer() else x for x in event] for event in envelope]

    envelope = remove_redundant_points(envelope)

    return envelope

def patterns_size_info(patterns):
    segments = [s for p in patterns for o in p["data"] for s in o]
    values = [x for s in segments for x in s]
    return (len(values), len(segments))

def generate_patterns(filepath):
    root = load_als(filepath)
    tempo = get_tempo(root)
    logging.info("Found tempo: %d", tempo)
    pointee_envelopes = read_envelopes(root)
    name_pointees = extract_macro_mappings(root)

    name_envelopes = {
        name: pointee_envelopes[pointee]
        for name, pointee in name_pointees.items()
        if pointee in pointee_envelopes
    }
    logging.info(f"Found {len(name_envelopes)} envelopes: {list(name_envelopes.keys())}")

    locators = find_locators(root)
    logging.debug(f"Found {len(locators)} locators: {locators}")

    patterns = {}
    for start_locator, end_locator in zip(locators, locators[1:]):
        start_name = start_locator[0]
        start_time = start_locator[1]
        # end_name = end_locator[0]
        end_time = end_locator[1]
        logging.debug(f"Cutting pattern {start_name} from {start_time} to {end_time}")
        patterns[start_name] = {
            envelope_name: cut_envelope(envelope, start_time, end_time)
            for envelope_name, envelope in name_envelopes.items()
        }
    # print(patterns["pulselowleft"])

    channel_order = [
        "T1L0","T1L1","T1L2","T1L3",
        "T2L0","T2L1","T2L2","T2L3",

        "R0","R1","R2","R3",
        "EO","E1","E2","E3",

        "G0","G1","G2","G3",
        "B0","B1","B2","B3",

        "W0","W1","W2","W3",
        "W4","W5","W6","W7"
    ]

    # sorted_envs = 
    to_save = [
        {
            "name" : name,
            "data": [
                sanitise_envelope(envelopes[c], tempo)
                for c in channel_order
            ]
        }
        for name, envelopes in patterns.items()
    ]

    to_save = [x for x in to_save if x["name"] not in ["s1", "long1", "long2", "strobetreeboth"]]
    # to_save = to_save[:20]
    logging.info(f"Loaded {len(to_save)} patterns")

    json_out = json.dumps(to_save, indent=2)
    open("patterns.json", "w").write(json_out)
    num_values, num_segments = patterns_size_info(to_save)
    logging.info(f"All patterns have {num_values} values in {num_segments} segments")
    return to_save
    # exit()



if __name__ == "__main__":
    if len(argv) < 2:
        logging.error("Usage: python readals.py <filepath>")
        exit()

    filepath = argv[1]
    print(generate_patterns(filepath))
