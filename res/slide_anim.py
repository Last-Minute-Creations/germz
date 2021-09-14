import random

# Generate data
coords = []
for x in range(16):
    for y in range(16):
        coords.append([x, y])
random.shuffle(coords)

# Print data
print('static const UBYTE s_pCoords[] = {')
i = 8
for c in coords:
    if i >= 8:
        print('\n\t', end='')
        i = 0
    print('0x{:X}{:X}, '.format(c[0], c[1]), end='')
    i += 1
print('\n};')
