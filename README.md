# Scena 3D OpenGL

## Sterowanie

### Samochód
Strzałki - jazda do przodu/do tyłu, obrót w lewo/prawo
O - podniesienie samochodu
L - opuszczenie samochodu
P - reflektory w górę
; - reflektory w dół

### Kamera
1 - kamera latająca
2 - kamera z trzeciej osoby na samochód
3 - kamera stała na samochód i flagę
WASD - ruch kamerą nr 1
Myszka - obrót kamery

### Pora dnia
N - noc
M - dzień

### Mgła
I - zwiększenie intensywności mgły
K - zmniejszenie intensywności mgły

### Składowa zwierciadlana
U - Blinn
J - Phong

### Okienko do debugowania
Y - pokaż
H - ukryj

## Zadanie 3.

### Flaga na wietrze (płat Beziera) (Tessellation Shader)

Flaga jest generowana z 16 punktów kontrolnych. Każdy punkt to współrzędna x, y, z i parametr u, v.
Wszystkie punkty są przekazywane do shadera w jednym patchu a następnie do Tessellation Evaluation Shader. W czasie teselacji są tworzone nowe wierzchołki i interpolowane są na nie wartości u i v z punktów kontrolnych.
Dla nowo wygenerowanych punktów pozycja jest wyliczana używając przekazanych punktów kontrolnych i zinterpolowanych wartości u i v z użyciem wielomianów Bernsteina.

### Blinn

Przełączanie między składową Phong a Blinn zaimplementowane we Fragment Shaderze używając flagi przekazanej jako uniform. Składowa zwierciadlana jest liczona w zależności od wybranej metody.