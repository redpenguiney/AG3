#version 430

in vec4 fragmentColor;
in vec3 fragmentNormal;
in vec3 cameraToFragmentPosition;
in vec3 cameraToFragmentInTangentSpace;
// in vec4 lightSpaceCoords;

#$INCLUDE$ "../shaders/phong_lighting.glsl"

layout(location = 0) out vec4 Output;

layout(binding=0) uniform sampler2DArray colorMap; // note: this syntax not ok until opengl 4.2
layout(binding=1) uniform sampler2DArray normalMap; // note: this syntax not ok until opengl 4.2
layout(binding=2) uniform sampler2DArray specularMap; // note: this syntax not ok until opengl 4.2
layout(binding=3) uniform sampler2DArray displacementMap; // note: this syntax not ok until opengl 4.2

uniform bool normalMappingEnabled;
uniform bool parallaxMappingEnabled;
uniform bool specularMappingEnabled;
uniform bool colorMappingEnabled;

#$INCLUDE$ "../shaders/parallax_mapping.glsl"

uniform float textureRepeat;
uniform vec3 triplanarCameraPosition;

vec4 TriplanarProjection(vec3 normal, sampler2DArray sampler) {
    vec3 blending = abs( normal );
	blending = normalize(max(blending, 0.00001)); // Force weights to sum to 1.0
	float b = (blending.x + blending.y + blending.z);
	blending /= vec3(b, b, b);

    vec3 pos = cameraToFragmentPosition + triplanarCameraPosition;
    vec4 xaxis = texture( sampler, vec3(pos.yz * textureRepeat, 0.0));
	vec4 yaxis = texture( sampler, vec3(pos.xz * textureRepeat, 0.0));
	vec4 zaxis = texture( sampler, vec3(pos.xy * textureRepeat, 0.0));
	vec4 normalTex = xaxis * blending.x + yaxis * blending.y + zaxis * blending.z;
	//normalTex = normalTex * 2.0 - 1.0;
	//normalTex.xy *= normalScale;
    //normalTex = normalize( normalTex );

	//vec3 T = vec3(0.,1.,0.);
  	//vec3 BT = normalize( cross( vNormal, T ) * 1.0 );

  	//mat3 tsb = mat3( normalize( T ), normalize( BT ), normalize( vNormal ) );
  	//vec3 N = tsb * normalTex;
    return normalTex;
    //return vec4(blending, 1.0);
}

// TODO; to avoid color banding add dithering 
void main()
{
    vec3 normal = normalize(fragmentNormal);
    //vec3 realTexCoords = TriplanarProjectionUvs(normal);
    //if (normalMappingEnabled) {normal = normalize(TBNmatrix * (texture(normalMap, realTexCoords).rgb * 2.0 - 1.0));} // todo: matrix multiplication in fragment shader is really bad, maybe?
    //if (normalMappingEnabled) {
      //  normal = 
    //}

    float specularStrength = 0.5;
    if (specularMappingEnabled) {
        specularStrength = TriplanarProjection(normal, specularMap).x;
    }

    vec3 light = CalculateLighting(specularStrength, normal);

    vec4 tx;
    //if (realTexCoords.z < 0) {
      //  tx = vec4(1.0, 1.0, 1.0, 1.0);
    //}a
    //else {
        tx = TriplanarProjection(normal, colorMap);
    //}
    

    //vec4 color = tx * vec4(light, 1);

    //vec4 color = vec4(light * tx.xyz, 1);
    Output = fragmentColor * vec4(light * tx.xyz, 1.0);

    if (Output.a < 0.1) {
        discard;
    };

    //Output = color;
    //Output = vec4(light, 1.0);
    //Output = vec4(spotLightOffset, 1.0, 1.0, 1.0);
    //Output = vec4(spotLights[0].directionAndOuterAngle.xyz, 1.0);
    //Output = vec4(normalize(fragmentNormal), 1.0);
    //Output = vec4(-envLightDirection, 1.0);
    //Output = vec4(fragmentColor.xyz, 1.0);
    //Output = vec4(1.0, 1.0, 1.0, 1.0);
    //Output = vec4(gl_FragCoord.www, 1.0);
    //Output = vec4(dot(normal, envLightDirection));

    //gl_FragDepth = 0.5;
};