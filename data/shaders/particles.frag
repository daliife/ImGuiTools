#version 330

uniform sampler2D u_diffuse_map;
//out color
out vec4 fragColor;


void main(){
    fragColor = texture(u_diffuse_map, gl_PointCoord);
}
