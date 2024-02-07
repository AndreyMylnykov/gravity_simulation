import subprocess
import math
import os
import numpy as np
from PIL import Image, ImageDraw
from moviepy.editor import ImageSequenceClip

def createEmptyFolder(folder_path):
    try:
        os.makedirs(folder_path, exist_ok=True)
        for file_name in os.listdir(folder_path):
            file_path = os.path.join(folder_path, file_name)
            try:
                if os.path.isfile(file_path):
                    os.unlink(file_path)
                elif os.path.isdir(file_path):
                    os.rmdir(file_path)
            except Exception as e:
                print(f"Error deleting {file_path}: {e}")
    except Exception as e:
        print(f"An error occurred: {e}")



def decodeArray(array):
    result = array.split("#")
    result.pop()
    for i in range(len(result)):
        result[i] = result[i].split("\n")
        for j in range(len(result[i])):
            result[i][j] = result[i][j].split("/")
            for k in range(len(result[i][j])):
                try:
                    result[i][j][k] = int(result[i][j][k])
                except:
                    if result[i][j][k] == '':
                        result[i].pop(j)
                    else:
                        print("ERROR!")
    return result

def clamp(x, minimum, maximum):
    if x < minimum:
        return minimum
    if x > maximum:
        return maximum
    return x


def kelvinToRGB(kelvin):
    temp = kelvin / 100
    red, green, blue = 0, 0, 0
    if temp == 0:
        return red, green, blue

    if temp <= 66:
        red = 255
        green = temp
        green = 99.4708025861 * math.log(green) - 161.1195681661

        if temp <= 19:
            blue = 0
        else:
            blue = temp - 10
            blue = 138.5177312231 * math.log(blue) - 305.0447927307

    else:
        red = temp - 60
        red = 329.698727446 * red ** -0.1332047592

        green = temp - 60
        green = 288.1221695283 * green ** -0.0755148492

        blue = 255

    if kelvin < 1000:
        red = 255 * (kelvin / 1000) * (kelvin / 1000)
    if green < 0:
        green = 0
        
    return ( round(red), round(green), round(blue) )

bodiesInChunk = 50000 #Depands on computer capabilities.
G = 0.3
bodies = 1000
num_frames = 1800
scrx = 1920
scry = 1080
r = 3.0
video_speed = 1.0
friction = 0.0095
#friction = 0.009
temperature_coefficient = 0.5
#temperature_coefficient = 0.5

callCPP = False
createImages = False
renderVideo = False

scene = "noise"
#scene = "planet.txt/-500/100/0/0#moon.txt/900/500/-10/0"
saveAs = "none"

if '/' in scene:
    bodies = 0
    a = scene.split('#')
    for i in range(len(a)):
        filename = "objects/" + a[i].split('/')[0]
        with open(filename, 'r') as file:
            bodies += sum(1 for line in file)

if callCPP:
    chunks = int(num_frames*bodies/bodiesInChunk)
    if chunks == 0:
        chunks = 1

    framesInChunk = int(num_frames / chunks)
    if framesInChunk < 1:
        framesInChunk = 1


    print(f"Chunks: {chunks}")
    print(f"Frames in chunk: {framesInChunk}")
    print(f"Number of frames: {num_frames}")
    #if framesInChunk * chunks != num_frames:
    #    print(f"Frames: {num_frames} ---> {framesInChunk*chunks}")
    #    num_frames = framesInChunk * chunks

    executable_path = r'GS2.exe'
    cpp_command = [executable_path, str(bodies), str(G), str(chunks), str(framesInChunk), str(scrx), str(scry),str(r),str(video_speed),str(friction),str(num_frames), str(temperature_coefficient), scene, saveAs]
    process = subprocess.Popen(cpp_command, stdout=subprocess.PIPE, text=True)

    while True:
        output = process.stdout.readline()
        if output == '' and process.poll() is not None:
            break
        if output:
            print(f"С++: {output.strip()}%")

frames_folder = "images"

if createImages:
    print("Rendering images...")

    createEmptyFolder(frames_folder)


    chunks_list = os.listdir('chunks')
    chunks_list = sorted(chunks_list, key=lambda x: int(x.split('_')[1].split('.')[0]))

    counter = 0
    goalPercentage = 0

    for chunk_path in chunks_list:
        chunk_data = ""
        with open('chunks/' + chunk_path, 'r') as file:
            chunk_data = file.read()

        frames = decodeArray(chunk_data)

        for frame in frames:
            percentage = int(( video_speed * counter / num_frames) * 100)
            if percentage > goalPercentage:
                print(f"Images: {percentage}%")
                goalPercentage += 1

            image = Image.new("RGB", (int(scrx), int(scry)), (50, 50, 50))
            draw = ImageDraw.Draw(image)

            for x, y, kelvins in frame:
                if 0 <= x < scrx and 0 <= y < scry:
                    draw.ellipse((x - r, y - r, x + r, y + r), fill=kelvinToRGB(kelvins))

            image.save(f"{frames_folder}/frame_{counter}.png")
            counter += 1

    print("Images: Done!")

if renderVideo:
    print("Rendering video...")
    frames_list = os.listdir(frames_folder)
    frames_list = sorted(frames_list, key=lambda x: int(x.split('_')[1].split('.')[0]))

    clip = ImageSequenceClip([f"{frames_folder}/{img}" for img in frames_list], fps=30)

    clip.write_videofile("output.mp4", codec='libx264', fps=30)
    print("Rendering: Done!")






# Размер изображения
width, height = 1000, 100

# Создание изображения
image = Image.new("RGB", (width, height))

# Заполнение изображения градиентом
for x in range(width):
    # Рассчитываем температуру для текущей позиции x
    temperature = x / width * 10000

    # Получаем цвет для температуры
    color = kelvinToRGB(temperature)

    # Заполняем столбец изображения этим цветом
    for y in range(height):
        image.putpixel((x, y), color)

# Сохранение изображения
image.save("temperature_gradient.png")
