import struct

def generate_bmp(filename, width, height):
    # 24 bpp, uncompressed
    padding = (4 - (width * 3) % 4) % 4
    row_stride = width * 3 + padding
    pixel_data_size = row_stride * height

    file_size = 54 + pixel_data_size

    with open(filename, 'wb') as f:
        # File Header
        f.write(b'BM')
        f.write(struct.pack('<I', file_size))
        f.write(struct.pack('<H', 0))
        f.write(struct.pack('<H', 0))
        f.write(struct.pack('<I', 54))

        # Info Header
        f.write(struct.pack('<I', 40))
        f.write(struct.pack('<i', width))
        f.write(struct.pack('<i', height))
        f.write(struct.pack('<H', 1))
        f.write(struct.pack('<H', 24))
        f.write(struct.pack('<I', 0)) # BI_RGB
        f.write(struct.pack('<I', pixel_data_size))
        f.write(struct.pack('<i', 2835))
        f.write(struct.pack('<i', 2835))
        f.write(struct.pack('<I', 0))
        f.write(struct.pack('<I', 0))

        # Write pixel data (Red square with green border)
        for y in range(height):
            for x in range(width):
                b = g = r = 0
                if x == 0 or x == width - 1 or y == 0 or y == height - 1:
                    g = 255 # Green border
                else:
                    r = 255 # Red center
                f.write(struct.pack('BBB', b, g, r))
            for _ in range(padding):
                f.write(b'\0')

generate_bmp("test.bmp", 16, 16)
