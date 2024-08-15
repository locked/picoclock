from PIL import Image

im = Image.open('2024-08-15_pixel.gif')

pixels = list(im.getdata())
width, height = im.size
lines = []
start_line = None
for x in range(width):
    for y in range(height):
        if pixels[x * height + y] == 0:
            print("x", end="")
            if start_line is None:
                start_line = [x, y]
        else:
            print(" ", end="")
            if start_line is not None:
                new_line = [start_line, [x, y]]
                lines.append(new_line)
                start_line = None
    print("")

#print(lines)

print ("line_struct light[]={")
i = 0
for line in lines:
    print("  {%d, %d, %d, %d}" % (line[0][0], line[0][1], line[1][0], line[1][1]), end="")
    i+=1
    last = i == len(lines)
    print("};" if last else ",")
