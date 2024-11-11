mkdir -p outline_png solid_png

for svg in outline_svg/*.svg; do
    filename=$(basename "$svg" .svg)
    inkscape -w 512 -h 512 "$svg" -o "outline_png/${filename}.png"
done

for svg in solid_svg/*.svg; do
    filename=$(basename "$svg" .svg)
    inkscape -w 512 -h 512 "$svg" -o "solid_png/${filename}.png"
done
