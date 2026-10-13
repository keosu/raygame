"""Original procedural temple scenery and frog launcher. Requires Pillow.

Paths mirror Model.h; regenerate after editing track knots. Uses 3x rasterization
and Lanczos filtering for smooth curved rails. No assets from the original game.
"""
import argparse
import math
import random
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--output", type=Path, default=Path(__file__).resolve().parents[1] / "assets")
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
scale = 3
paths = [
    [(74,178),(352,156),(757,147),(1152,173),(1170,354),(1134,526),(922,579),(709,559),(533,515),(306,529),(130,597)],
    [(80,560),(139,350),(92,184),(375,149),(834,161),(1158,240),(1118,453),(873,576),(410,598),(352,476),(388,323),(809,287),(941,364),(872,456)],
    [(67,160),(563,160),(1147,170),(1180,525),(826,585),(316,579),(111,451),(159,281),(424,239),(817,241),(968,335),(961,466),(730,526),(384,507),(309,403)],
]
palettes = [((34,67,54),(17,34,31),(133,146,99)), ((35,57,83),(14,30,49),(140,161,184)), ((76,49,49),(32,26,37),(185,147,95))]
def curve(knots):
    points = []
    for i in range(len(knots)-1):
        a,b,c,d = knots[max(0,i-1)],knots[i],knots[i+1],knots[min(i+2,len(knots)-1)]
        for step in range(24):
            t=step/24
            points.append(tuple(0.5*(2*b[j]+(-a[j]+c[j])*t+(2*a[j]-5*b[j]+4*c[j]-d[j])*t*t+(-a[j]+3*b[j]-3*c[j]+d[j])*t*t*t) for j in range(2)))
    return points+[knots[-1]]
for stage,knots in enumerate(paths):
    rng=random.Random(705+stage)
    top,bottom,stone=palettes[stage]
    image=Image.new("RGBA",(1280*scale,720*scale))
    draw=ImageDraw.Draw(image)
    for y in range(90*scale,680*scale):
        t=(y/scale-90)/590
        color=tuple(round(a*(1-t)+b*t) for a,b in zip(top,bottom))+(255,)
        draw.line((28*scale,y,1252*scale,y),fill=color)
    def line(points,color,width):
        coords=[(round(x*scale),round(y*scale)) for x,y in points]
        draw.line(coords,fill=color,width=round(width*scale),joint="curve")
        radius=width*scale/2
        for x,y in coords[::6]:
            draw.ellipse((x-radius,y-radius,x+radius,y+radius),fill=color)
    # Weathered stone paving, scratches and fine grain.
    for y in range(112,650,84):
        for x in range(42,1230,126):
            shift=35 if (y//84)%2 else 0
            r=((x+shift)*scale,y*scale,(x+shift+112)*scale,(y+70)*scale)
            draw.rounded_rectangle(r,radius=7*scale,outline=tuple(min(255,c+10) for c in bottom)+(255,),width=scale)
            if rng.random()<0.5:
                line([(x+shift+10,y+10),(x+shift+23,y+15),(x+shift+19,y+29)],tuple(min(255,c+5) for c in bottom)+(255,),1)
    for _ in range(16000):
        x,y=rng.randint(32,1246)*scale,rng.randint(94,676)*scale
        draw.point((x,y),fill=tuple(min(255,c+16) for c in bottom)+(110,))
    # Leaf clusters at the perimeter, with thin stems and leaf veins.
    for i in range(24):
        x=rng.choice([rng.uniform(42,120),rng.uniform(1170,1230)])
        y=rng.uniform(130,640)
        for j in range(5):
            a=rng.uniform(-math.pi,math.pi)
            end=(x+math.cos(a)*rng.uniform(18,36),y+math.sin(a)*rng.uniform(14,30))
            line([(x,y),end],(64,98,65,255),2)
            ex,ey=end
            draw.ellipse(((ex-11)*scale,(ey-6)*scale,(ex+11)*scale,(ey+6)*scale),fill=(67+stage*8,112-stage*9,79,255))
            line([(ex-8,ey),(ex+8,ey)],(101,139,92,255),0.7)
    points=curve(knots)
    shadow=Image.new("RGBA",image.size)
    sd=ImageDraw.Draw(shadow)
    sd.line([(int((x+4)*scale),int((y+8)*scale)) for x,y in points],fill=(0,4,12,170),width=56*scale,joint="curve")
    image=Image.alpha_composite(image,shadow.filter(ImageFilter.GaussianBlur(5*scale)))
    draw=ImageDraw.Draw(image)
    for width,color in [(50,(12,20,23,255)),(47,stone+(255,)),(42,tuple(int(c*0.76) for c in stone)+(255,)),(34,(24,35,36,255)),(30,(43,57,50,255))]:
        line(points,color,width)
    # Only mark the outside rails: the trough is continuous beneath the marbles.
    accumulated=0
    for i in range(1,len(points)):
        x,y=points[i]; px,py=points[i-1]
        length=math.hypot(x-px,y-py)
        accumulated+=length
        if accumulated>70 and length>0:
            accumulated=0
            nx,ny=-(y-py)/length,(x-px)/length
            for sign in (-1,1):
                line([(x+nx*18*sign,y+ny*18*sign),(x+nx*23*sign,y+ny*23*sign)],(35,45,42,255),1.5)
    image.resize((2560,1440),Image.Resampling.LANCZOS).save(args.output/f"temple-{stage}.png")

# A polished brass plinth and a green stone frog, facing right; rotated at runtime.
size=768
image=Image.new("RGBA",(size,size)); draw=ImageDraw.Draw(image)
def ellipse(box,color): draw.ellipse(tuple(int(v*6) for v in box),fill=color)
ellipse((4,9,124,125),(0,7,13,70))
for radius in range(58,0,-1):
    t=radius/58
    ellipse((64-radius,64-radius,64+radius,64+radius),(int(95+64*(1-t)),int(83+59*(1-t)),int(40+35*(1-t)),255))
draw.ellipse((9*6,9*6,119*6,119*6),outline=(218,194,116,255),width=2*6)
ellipse((25,22,101,110),(22,54,36,255))
ellipse((26,22,97,103),(48,115,70,255))
ellipse((30,25,78,83),(69,143,79,255))
ellipse((59,38,117,92),(54,114,63,255))
ellipse((83,44,119,88),(20,55,34,255))
ellipse((86,48,116,84),(8,29,23,255))
for cy in (30,98):
    ellipse((57,cy-14,87,cy+14),(24,62,36,255))
    ellipse((59,cy-13,86,cy+11),(139,175,82,255))
    ellipse((66,cy-9,83,cy+7),(249,237,184,255))
    ellipse((73,cy-7,82,cy+5),(13,29,23,255))
    ellipse((74,cy-6,77,cy-3),(255,255,236,255))
for y in (43,54,67,79): ellipse((37,y,43,y+4),(42,91,53,255))
image.resize((256,256),Image.Resampling.LANCZOS).save(args.output/"launcher.png")
print("Generated three temple backdrops and frog launcher")
