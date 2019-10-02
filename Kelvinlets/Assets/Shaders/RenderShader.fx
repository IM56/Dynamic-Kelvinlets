static const float PI = 3.14159265f;

struct Displacement
{
	float3 vertexDisplacement;
	float3 neighbourDisplacement1;
	float3 neighbourDisplacement2;
};

cbuffer PerFrameCB : register(b0)
{
	matrix matProjection;
	matrix matView;
	matrix matProjInv;
	matrix matViewInv;
	float3 camPos;
	float  deltaTime;	// deltaTime is given in seconds
	float elapsedTime;
	bool normalCorrect;
	float2 FCB_padding;
};

cbuffer PerInstanceCB : register(b1)
{
	matrix matModel;
	uint numVertices;
	uint numKelvinlets;
	float alpha;
	float beta;
};

cbuffer PointLightCB : register(b2)
{
	float3 lightPos;
	float power;
	float3 lightCol;
	float angularFrequency;
};

Texture2D texture0 : register(t0);

SamplerState linearMipSampler : register(s0);

struct VertexInput
{
	float3 pos   : POSITION;
	float4 color : COLOUR;
	float3 normal : NORMAL;
	float4 tangent : TANGENT;
	float2 uv : TEXCOORD;
};

struct VertexOutput
{
	float4 vpos  : SV_POSITION;
	float4 color : COLOUR;
	float3 normal : NORMAL;
	float4 tangent : TANGENT;
	float2 uv : TEXCOORD;
	float3 worldPos : TEXCOORD1;
};

// Buffer of Kelvinlet displacements for each vertex
StructuredBuffer<Displacement> displacements : register(t1);

VertexOutput VS_Main(VertexInput input, uint vertexID : SV_VertexID)
{
	VertexOutput output;
	output.color = input.color;
	output.tangent = input.tangent;
	output.uv = input.uv;
	output.vpos = mul(float4(input.pos, 1.0f), matModel);

	// Find two local points in the vertex's tangent plane
	float3 bitangent = cross(input.normal, input.tangent.xyz);
	float3 localPos1 = output.vpos.xyz + 0.1f * input.tangent.xyz;
	float3 localPos2 = output.vpos.xyz + 0.1f * bitangent;

	// Add the corresponding Kelvinlet displacements to the vertex and its neighbours
	output.vpos.xyz += displacements[vertexID].vertexDisplacement;
	localPos1 += displacements[vertexID].neighbourDisplacement1;
	localPos2 += displacements[vertexID].neighbourDisplacement2;

	// Estimate the tangent plane after displacement
	float3 estimatedNormal = normalize(cross(localPos1 - output.vpos.xyz, localPos2 - output.vpos.xyz));

	// Create matrix for sending the displaced vertex to screen space
	matrix matVP = mul(matView, matProjection);

	// Project the displaced vertex position to screen space
	output.worldPos = output.vpos;
	output.vpos = mul(float4(output.vpos.xyz, 1.0f), matVP);

	// If the size of the displacement is sufficiently large, adjust the vertex normal
	float3 localDisplacement1 = displacements[vertexID].neighbourDisplacement1;
	float3 localDisplacement2 = displacements[vertexID].neighbourDisplacement2;
	float scaleFactor = (length(localDisplacement1) + length(localDisplacement2)) / 2.0f;
	
	[branch] if (normalCorrect && scaleFactor > 0.1f)
		output.normal = estimatedNormal;
		//output.normal = (input.normal + estimatedNormal) * 0.5f;
	else
		output.normal = input.normal;  

	return output;
}

float4 PS_Main(VertexOutput input) : SV_TARGET
{
	float3 ambientCol = float3(0.0f, 0.0f, 0.5f);
	float3 diffuseCol = lightCol;
	float3 specularCol = float3(1.0f, 1.0f, 1.0f);
	float shininess = 128.0f;

	float3 N = input.normal;
	
	// Find the direction of the light shining on the surface
	//float3 L = lightPos - input.worldPos;

	// Define an orbiting point light
	// Number of completed orbits = orbits per second * time elapsed in seconds
	float theta = (angularFrequency * elapsedTime);
	float3 L = float3(30.0f*cos(theta), 20.0f, 30.0f*sin(theta)) - input.worldPos;
	// Store the distance before normalizing the light vector
	float distanceSq = dot(L, L);
	L /= sqrt(distanceSq);
	// Find the reflection vector
	float3 R = normalize(2.0f * N * dot(N,L) - L);
	// Find the view vector
	float3 V = normalize(camPos - input.worldPos);
	// Find the half vector
	float3 H = normalize(V + L);
	
	float NdotL = dot(N, L);
	float4 lighting = float4(0.0f, 0.0f, 0.0f, 0.0f);
	
	if (NdotL >= 0.0f)
	{
		float lambertian = saturate(NdotL);
		float att = 1.0f / (1.0f + 0.1f*distanceSq);
		float specular = pow(saturate(dot(H, N)), shininess);

		lighting = float4(att * power*lightCol*(diffuseCol*lambertian + specularCol * specular), 1.0f);
	}
	
	return float4(ambientCol, 1.0f) + lighting;
}
