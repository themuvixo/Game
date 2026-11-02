// Post-processing: vignette, gentle bloom-ish lift and colour grade. GLSL 1.10 (SFML 2.6 compatible).
uniform sampler2D texture;
uniform float time;
uniform float vignette;
uniform vec3 tint;
uniform float flash;
void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    vec4 c = texture2D(texture, uv);
    vec2 px = vec2(1.0 / 1280.0, 1.0 / 720.0) * 2.0;
    vec4 blur = (texture2D(texture, uv + vec2(px.x, 0.0)) + texture2D(texture, uv - vec2(px.x, 0.0)) +
                 texture2D(texture, uv + vec2(0.0, px.y)) + texture2D(texture, uv - vec2(0.0, px.y))) * 0.25;
    float lum = dot(blur.rgb, vec3(0.299, 0.587, 0.114));
    c.rgb += blur.rgb * smoothstep(0.65, 1.0, lum) * 0.35;
    c.rgb *= tint;
    vec2 d = uv - vec2(0.5);
    float v = smoothstep(0.85, 0.25, length(d) * (1.0 + vignette));
    c.rgb *= mix(1.0, v, 0.55);
    c.rgb = mix(c.rgb, vec3(1.0), flash);
    gl_FragColor = vec4(c.rgb, 1.0) * gl_Color;
}
