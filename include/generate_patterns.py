from random import randint
import numpy as np

def list_of_arrays_to_cpp_file(arrays, filename):
    num_patterns = len(arrays)
    num_outputs = arrays[0].shape[1]
    
    VALUE_TYPE = "uint16_t"
    with open(filename, 'w') as f:
        f.write("#ifndef FADER_PATTERNS_H\n")
        f.write("#define FADER_PATTERNS_H\n\n")
        f.write("#include <cstdlib>\n\n")
        f.write("// Define constants\n")
        f.write(f"#define FADER_PATTERNS_NUM {num_patterns}\n")
        f.write(f"#define FADER_PATTERN_OUTPUTS_NUM {num_outputs}\n\n")
        
        for i, array in enumerate(arrays):
            f.write(f"static const {VALUE_TYPE} FADER_PATTERN_{i+1}[][FADER_PATTERN_OUTPUTS_NUM] = {{\n")
            for row in array:
                row_str = ", ".join(map(str, row))
                f.write(f"    {{{row_str}}},\n")
            f.write("};\n\n")
        
        f.write("// Create array PATTERNS containing all patterns\n")
        f.write(f"static const {VALUE_TYPE}* const FADER_PATTERNS[] = {{\n")
        for i in range(num_patterns):
            f.write(f"    (const {VALUE_TYPE}* const)FADER_PATTERN_{i+1},\n")
        f.write("};\n\n")

        f.write("// Create array indicating the length of each pattern\n")
        f.write(f"static const {VALUE_TYPE} FADER_PATTERN_LENGTHS[FADER_PATTERNS_NUM] = {{\n")
        f.write(f"    {', '.join([str(len(array)) for array in arrays])},\n")
        # for length in pattern_lengths:
        #     f.write(f"    {length},\n")
        f.write("};\n\n")
        
        f.write("#endif // FADER_PATTERNS_H\n")
# Example usage
if __name__ == "__main__":
    # Create a list of numpy arrays with random values
    # arrays = [
    #     np.random.randint(0, 4095, size=(randint(3,10), 16))
    #     for i in range(randint(2, 8))
    # ]

    arrays = []
    for i in range(10):
        pattern_length = randint(20, 100)
        pulse_length = randint(10, pattern_length)
        # create a fade down from 4095 to 0 over pulse_length
        fade_down = np.linspace(4095, 0, pulse_length, dtype=np.uint16)
        # extend it to pattern_length with zeros
        fade_down = np.concatenate((fade_down, np.zeros(pattern_length - pulse_length, dtype=np.uint16)))
        # repeat it for 16 columns
        fade_down = np.tile(fade_down, (16, 1)).T
        arrays.append(fade_down)
    # Convert the list of numpy arrays to a C++ file
    list_of_arrays_to_cpp_file(arrays, "FaderPatterns.h")
