#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <fstream>
#include "DDSTextureLoader.h"

using namespace DirectX;
using namespace std;

const int TEXTURE_REPEAT = 8;

class TerrainClass
{
public:
	enum GENERATIONTYPE
	{
		NONE,
		HEIGHTMAP,
		VoxelTerrain
	};

private:
	struct VertexType
	{
		XMFLOAT3 position;
		XMFLOAT2 texture;
		XMFLOAT3 normal;
	};

	struct HeightMapType
	{
		float x;
		float y;
		float z;

		float tu;
		float tv;

		float nx;
		float ny;
		float nz;
	};

	struct VectorType 
	{
		float x;
		float y;
		float z;
	};

public:
	TerrainClass();
	TerrainClass(const TerrainClass&);
	~TerrainClass();

	bool Initialize(ID3D11Device*, char *, WCHAR*);
	bool InitializeHeight(ID3D11Device*, char *, WCHAR*);
	bool InitializeDS(ID3D11Device*, char *, WCHAR*);
	void Shutdown();
	void Render(ID3D11DeviceContext*);

	int GetIndexCount();
	int GetVertexCount();

	void SetGenerationType(GENERATIONTYPE type) { m_GenType = type; }

	ID3D11ShaderResourceView* GetTexture() { return m_Texture; }

	bool GetHeightAtPosition(float, float, float&);

	void ResetTerrain(ID3D11DeviceContext* deviceContext); 
	void ApplyDiamondSquare(ID3D11DeviceContext* deviceContext, float height = 30.0f);
	void ApplyFaultLine(ID3D11DeviceContext* deviceContext, int iterations, float height = 2.0f);
	void ApplyHillCircle(ID3D11DeviceContext* deviceContext, int iterations, float minheight = -50.0f, float maxheight = 50.0f);
	void ApplyParticleDeposition(ID3D11DeviceContext* deviceContext, int numIt);
	void ApplySmoothing(ID3D11DeviceContext* deviceContext, float factor);

	void SetWireFrame(bool iswireframe) { m_IsWireFrame = iswireframe; }

private:
	bool LoadHeightMap(char*);
	void NormalizeHeightMap();
	void ShutdownHeightMap();

	void CalculateTextureCoordinates();
	bool LoadTexture(ID3D11Device*, WCHAR*);
	void ReleaseTexture();

	bool InitializeBuffers(ID3D11Device*);
	bool InitializeDSBuffers(ID3D11Device*);

	void DiamondSquare(int row, int col, int size, float offset);
	void ParticleDeposit(int x, int z);

	float RandomFloat(float low, float high);

	void ShutdownBuffers();

	bool CheckHeightOfTriangle(float, float, float&, float[3], float[3], float[3]);

	bool CalculateNormals();

	int m_TerrainWidth;
	int m_TerrainHeight;
	int m_VertexCount;
	int m_IndexCount;

	int m_DSDivisions = 128;
	float m_DSSize = 300.0f;

	ID3D11ShaderResourceView* m_Texture;

	VectorType* m_vertexList;
	VertexType* vertices;
	ID3D11RasterizerState* m_WireframeRS;
	ID3D11RasterizerState* m_SolidRS;
	unsigned long* indices;
	bool m_IsWireFrame;

	HeightMapType* m_HeightMap;

	ID3D11Buffer *m_VertexBuffer;
	ID3D11Buffer *m_IndexBuffer;

	GENERATIONTYPE m_GenType;
};
