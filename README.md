# simpix

C++ starter code
* simpix_start.cpp
use make to build this example

Usage: simapix_start image1 image2 <output=out.png>

Python starter code
* simpix_start.py

Usage: simapix_start image1 image2 <output=out.png>


How to run:
make clean
make
./simpix -s pair11.jpg   -t pair12.jpeg -o pair11_to_pair12.png --moves 20000000 --melt 2000000 --t_start 50000 --t_end 10 --radius 50
python3 png_to_pdf.py pair11_to_pair12.png pair11_to_pair12.pdf

./simpix -s pair12.jpeg  -t pair11.jpg  -o pair12_to_pair11.png --moves 20000000 --melt 2000000 --t_start 50000 --t_end 10 --radius 50
python3 png_to_pdf.py pair12_to_pair11.png pair12_to_pair11.pdf

./simpix -s pair21.jpg -t pair22.jpg -o pair21_to_pair22.png --moves 20000000 --melt 2000000 --t_start 50000 --t_end 10 --radius 50
python3 png_to_pdf.py pair21_to_pair22.png pair21_to_pair22.pdf

./simpix -s pair22.jpg -t pair21.jpg -o pair22_to_pair21.png --moves 20000000 --melt 2000000 --t_start 50000 --t_end 10 --radius 50
python3 png_to_pdf.py pair22_to_pair21.png pair22_to_pair21.pdf

Execution time:
pair11 -> pair12: 2.95224 s
pair12 -> pair11: 2.92258 s
pair21 -> pair22: 2.94962 s
pair22 -> pair21: 2.90677 s


Used Images are from FreePic website, I searched for 1920*1080 Impressionism images, found images and resized them to fit 1920*1080.