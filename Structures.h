#pragma once
#include <directxmath.h>

using namespace DirectX;
struct SimpleVertex
{
	XMFLOAT3 PosL;
	XMFLOAT3 NormL;
	XMFLOAT2 Tex;

	bool operator<(const SimpleVertex other) const
	{
		return memcmp((void*)this, (void*)&other, sizeof(SimpleVertex)) > 0;
	}
};

struct Frame
{
	XMFLOAT3 Postion;
	XMFLOAT3 Rotation;
	XMFLOAT3 Scale;

	int frametime;
	int bone;

	Frame()
	{
		XMFLOAT3 Postion = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 Scale = XMFLOAT3(0.0f, 0.0f, 0.0f);

		frametime = -1.0f;
	}
};

struct SurfaceInfo
{
	XMFLOAT4 AmbientMtrl;
	XMFLOAT4 DiffuseMtrl;
	XMFLOAT4 SpecularMtrl;
};

struct Light
{
	XMFLOAT4 AmbientLight;
	XMFLOAT4 DiffuseLight;
	XMFLOAT4 SpecularLight;

	float SpecularPower;
	XMFLOAT3 LightVecW;
};

struct ConstantBuffer
{
	XMMATRIX World;
	XMMATRIX View;
	XMMATRIX Projection;

	SurfaceInfo surface;

	Light light;

	XMFLOAT3 EyePosW;
	float HasTexture;
};



