#version 330 core

in vec2 UV;

out vec3 color;

uniform sampler2D renderedTexture;

void main() {
    // Compute number of pixels to turn on in ink r, g, b
    vec3 c = texture(renderedTexture, UV).xyz;
    float on = max(max(c.x, c.y), c.z) * 9;
    int r = int(round(on * c.x / (c.x + c.y + c.z)));
    int g = int(round(on * c.y / (c.x + c.y + c.z)));
    int b = int(round(on * c.z / (c.x + c.y + c.z)));

    // The rendering order for ligtened r,g,b pixels in 3 * 3 mega pixel is defined to be first from left to right
    // then from bottom to top.
    // In the scheme, red: 1, green: 2, blue: 3, off: 0
    int scheme[9];
    int i = 0;

    while (r > 0) {
        scheme[i] = 1;
        r--;
        i++;
    }
    while (g > 0) {
        scheme[i] = 2;
        g--;
        i++;
    }
    while (b > 0) {
        scheme[i] = 3;
        b--;
        i++;
    }
    while (i < 9) {
        scheme[i] = 0;
        i++;
    }

    // Compute position of this pixel in a 3 * 3 mega pix
    int col = int(floor(gl_FragCoord.x)) % 3;
    int row = int(floor(gl_FragCoord.y)) % 3;

    // Find the pix in scheme
    int index = row * 3 + col;
    if (scheme[index] == 0) {
        color = vec3(0, 0, 0);
    } else if (scheme[index] == 1) {
        color = vec3(1, 0, 0);
    } else if (scheme[index] == 2) {
        color = vec3(0, 1, 0);
    } else {
        color = vec3(0, 0, 1);
    }
}
