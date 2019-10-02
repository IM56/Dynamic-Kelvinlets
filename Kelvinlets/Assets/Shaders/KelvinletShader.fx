//////////////////////
// KELVINLET SHADER //
//////////////////////

static const float PI = 3.14159265f;

// Kelvinlet data
struct Kelvinlet
{
	float3 loadCentre;		// Where to apply the Kelvinlet
	float epsilon;			// Regularization scale
	float3 forceParams;		// For impulses: force vector, for affines: scale factors.
	float startTime;		// Where the Kelvinlet starts in the timeline
	float age;				// Current timepoint of Kelvinlet's evolution
	float lifespan;			// How long the Kelvinlet will live
	int type;				// Is the Kelvinlet Impulse, Scale or Pinch?
};

// Vertex data
struct Vertex
{
	float3 pos;
	uint color;
	float3 normal;
	float4 tangent;
	float2 uv;
};

// Vertex displacements
struct Displacement
{
	float3 vertexDisplacement;		// Vertex's main displacement
	float3 neighbourDisplacement1;	// Estimated displacement of neighbour
	float3 neighbourDisplacement2;	// Estimated displacement of neighbour
};

// Per frame data
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

// Per instance data
cbuffer PerInstanceCB : register(b1)
{
	matrix matModel;	// Model matrix
	uint numVertices;	// Number of vertices in this instance
	uint numKelvinlets; // Number of kelvinlets attached to this instance
	float alpha;		// Material parameter: pressure wave speed
	float beta;			// Material parameter: shear wave speed
};

StructuredBuffer<Vertex> vertices : register(t0);
StructuredBuffer<Kelvinlet> kelvinlets : register(t1);
RWStructuredBuffer<Displacement> displacements : register(u0);

/////////////////////////////////////
// Kelvinlet displacement function //
/////////////////////////////////////

float3 impulse(float3 vpos, Kelvinlet k)
{
	// Get the vector from the load centre to current vertex
	float3 rVec = vpos - k.loadCentre;		//	r_ = x_ - c_
	float r = length(rVec);					//  r = |r_|

	// Work out components for dyadic product R = r(X)r = rr^T
	float3x3 R = { rVec.x*rVec.x, rVec.x*rVec.y, rVec.x*rVec.z,
				   rVec.y*rVec.x, rVec.y*rVec.y, rVec.y*rVec.z,
				   rVec.z*rVec.x, rVec.z*rVec.y, rVec.z*rVec.z };

	float2 ab_t = float2(alpha, beta) * k.age;
	float e = k.epsilon;

	// Calculate multiplicative constants for convenience
	float2 k_ab = (1.0f / (16.0f*PI*pow(r, 3))).xx / float2(alpha, beta);
	// Important auxiliary quantities
	float4 s_ab = { r + ab_t.x, r - ab_t.x, r + ab_t.y, r - ab_t.y };
	float4 s_ab_e = sqrt(s_ab*s_ab + e * e);

	// Avoid singularities in the limit r -> 0
	[branch] if (r < 0.0001f)
	{
		float2 ab_t_e = sqrt(ab_t*ab_t + e * e);
		return k.forceParams * 5.0f * k.age * pow(e, 4) * (1.0f / pow(ab_t_e.x, 7) + 2.0f / pow(ab_t_e.y, 7)) / (8.0f*PI);
	}
	else
	{
		// Pseudo-potentials used for evaluating displacement 
		float4 W = (2 * s_ab*s_ab + e * e - 3 * r*s_ab) / s_ab_e + r * pow(s_ab, 3) / pow(s_ab_e, 3);
		float4 dW = -3.0f*r*pow(e, 4) / pow(s_ab_e, 5);

		float2 U = k_ab * float2((W.x - W.y), (W.z - W.w));
		float2 dU = k_ab * float2((dW.x - 3.0f * W.x / r - dW.y + 3.0f * W.y / r),
			(dW.z - 3.0f * W.z / r - dW.w + 3.0f * W.w / r));

		float A = U.x + 2 * U.y + r * dU.y;
		float B = (dU.x - dU.y) / r;

		// 3x3 identity matrix
		float3x3 I = { 1, 0, 0,
						0, 1, 0,
						0, 0, 1 };

		float3x3 D = A * I + B * R;
		return mul(k.forceParams, D);
	}

}

float3 pinch(float3 vpos, Kelvinlet k)
{
	// Get the vector from the load centre to current vertex
	float3 rVec = vpos - k.loadCentre;		//	r_ = x_ - c_
	float r = length(rVec);					//  r = |r_|

	// Work out components for dyadic product R = r(X)r = rr^T
	float3x3 R = { rVec.x*rVec.x, rVec.x*rVec.y, rVec.x*rVec.z,
				   rVec.y*rVec.x, rVec.y*rVec.y, rVec.y*rVec.z,
				   rVec.z*rVec.x, rVec.z*rVec.y, rVec.z*rVec.z };

	float2 ab_t = float2(alpha, beta) * k.age;
	float e = k.epsilon;

	// Calculate multiplicative constants for convenience
	float2 k_ab = (1.0f / (16.0f*PI*pow(r, 3))).xx / float2(alpha, beta);
	// Important auxiliary quantities
	float4 s_ab = { r + ab_t.x, r - ab_t.x, r + ab_t.y, r - ab_t.y };
	float4 s_ab_e = sqrt(s_ab*s_ab + e * e);

	// Pseudo-potentials used for evaluating displacement 
	float4 W = (2 * s_ab*s_ab + e * e - 3 * r*s_ab) / s_ab_e + r * pow(s_ab, 3) / pow(s_ab_e, 3);
	float4 dW = -3.0f*r*pow(e, 4) / pow(s_ab_e, 5);
	float4 d2W = -3.0f * pow(e, 4) * (s_ab_e*s_ab_e - 5 * r * s_ab) / pow(s_ab_e, 7);

	// Auxiliary quantities for calculating displacement
	float2 U = k_ab * float2((W.x - W.y), (W.z - W.w));
	float2 dU = k_ab * float2((dW.x - 3 * W.x / r - dW.y + 3 * W.y / r), (dW.z - 3 * W.z / r - dW.w + 3 * W.w / r));
	float2 d2U = k_ab * float2((d2W.x - d2W.y - 6 * (dW.x - dW.y) / r + 12 * (d2W.x - d2W.y) / r),
		(d2W.z - d2W.w - 6 * (dW.z - dW.w) / r + 12 * (d2W.z - d2W.w) / r));

	float B = (dU.x - dU.y) / r;
	float dA = dU.x + 3.0f*dU.y + r * d2U.y;
	float dB = (d2U.x - d2U.y - B) / r;

	float3x3 pinchMatrix = { k.forceParams.x, 0, 0,      0, k.forceParams.y, 0,     0, 0, k.forceParams.z };
	float3 Fr = mul(pinchMatrix, rVec);
	return (dA / r + B) * Fr + dB * mul(rVec, Fr) * normalize(rVec);
}

float3 scale(float3 vpos, Kelvinlet k)
{
	// Get the vector from the load centre to current vertex
	float3 rVec = vpos - k.loadCentre;		//	r_ = x_ - c_
	float r = length(rVec);					//  r = |r_|

	// Work out components for dyadic product R = r(X)r = rr^T
	float3x3 R = { rVec.x*rVec.x, rVec.x*rVec.y, rVec.x*rVec.z,
				   rVec.y*rVec.x, rVec.y*rVec.y, rVec.y*rVec.z,
				   rVec.z*rVec.x, rVec.z*rVec.y, rVec.z*rVec.z };

	float2 ab_t = float2(alpha, beta) * k.age;
	float e = k.epsilon;

	// Calculate multiplicative constants for convenience
	float2 k_ab = (1.0f / (16.0f*PI*pow(r, 3))).xx / float2(alpha, beta);
	// Important auxiliary quantities
	float4 s_ab = { r + ab_t.x, r - ab_t.x, r + ab_t.y, r - ab_t.y };
	float4 s_ab_e = sqrt(s_ab*s_ab + e * e);

	// Pseudo-potentials used for evaluating displacement 
	float4 W = (2 * s_ab*s_ab + e * e - 3 * r*s_ab) / s_ab_e + r * pow(s_ab, 3) / pow(s_ab_e, 3);
	float4 dW = -3.0f*r*pow(e, 4) / pow(s_ab_e, 5);
	float4 d2W = -3.0f * pow(e, 4) * (s_ab_e*s_ab_e - 5 * r * s_ab) / pow(s_ab_e, 7);

	// Auxiliary quantities for calculating displacement
	float2 U = k_ab * float2((W.x - W.y), (W.z - W.w));
	float2 dU = k_ab * float2((dW.x - 3 * W.x / r - dW.y + 3 * W.y / r), (dW.z - 3 * W.z / r - dW.w + 3 * W.w / r));
	float2 d2U = k_ab * float2((d2W.x - d2W.y - 6 * (dW.x - dW.y) / r + 12 * (d2W.x - d2W.y) / r),
		(d2W.z - d2W.w - 6 * (dW.z - dW.w) / r + 12 * (d2W.z - d2W.w) / r));

	float B = (dU.x - dU.y) / r;
	float dA = dU.x + 3.0f*dU.y + r * d2U.y;
	float dB = (d2U.x - d2U.y - B) / r;

	return (4.0f*B + dA / r + r * dB) * k.forceParams.x * rVec;
}


////////////////////
// Compute shader //
////////////////////

[numthreads(256, 1, 1)]
void CS_Kelvinlet(uint3 DispatchThreadID : SV_DispatchThreadID)
{
	// Get current thread ID
	uint myID = DispatchThreadID.x;

	// Prevent undefined behaviour by not utilising more threads than vertices
	if (myID < numVertices)
	{
		// Read a vertex position and transform to world space
		Vertex vertex = vertices[myID];
		float3 vpos = mul(float4(vertex.pos, 1.0f), matModel).xyz;
		
		// Find local points in the vertex's tangent plane
		float3 bitangent = cross(vertex.normal, vertex.tangent.xyz);
		float3 localPos1 = vpos + 0.001f * vertex.tangent.xyz;
		float3 localPos2 = vpos + 0.001f * bitangent;

		float3 D1, D2, D3;
		
		Kelvinlet kelvinlet;

		displacements[myID].vertexDisplacement = float3(0.0f, 0.0f, 0.0f);
		displacements[myID].neighbourDisplacement1 = float3(0.0f, 0.0f, 0.0f);
		displacements[myID].neighbourDisplacement2 = float3(0.0f, 0.0f, 0.0f);

		// Determine the displacement each Kelvinlet causes for this vertex and accumulate
		for (uint i = 0; i < numKelvinlets; ++i)
		{
			// Read Kelvinlet data
			kelvinlet = kelvinlets[i];
			float3 d1, d2, d3;

			switch (kelvinlet.type)
			{
			case 1:		// Impulse
				d1 = impulse(vpos, kelvinlet);
				d2 = impulse(localPos1, kelvinlet);
				d3 = impulse(localPos2, kelvinlet);
				break;
			case 2:		// Pinch
				d1 = pinch(vpos, kelvinlet);
				d2 = pinch(localPos1, kelvinlet);
				d3 = pinch(localPos2, kelvinlet);
				break;
			case 3:		// Scale
				d1 = scale(vpos, kelvinlet);
				d2 = scale (localPos1, kelvinlet);
				d3 = scale(localPos2, kelvinlet);
				break;
			default:
				d1 = d2 = d3 = float3(0.0f, 0.0f, 0.0f);
				break;
			}
			
			D1 += d1;
			D2 += d2;
			D3 += d3;	
		}
		displacements[myID].vertexDisplacement = D1;
		displacements[myID].neighbourDisplacement1 = D2;
		displacements[myID].neighbourDisplacement2 = D3;
	}
}
