#pragma once
#include <vector>
#include "GameObject.h"
#include "Structures.h"
#include "libnoise\include\noise\noise.h"

using namespace noise;

class VoxelTerrain
{
public:
	VoxelTerrain();
	~VoxelTerrain();
	void SetCubeGeometry(Geometry geomtry) { m_CubeGeometry = geomtry; }
	void SetMaterial(Material material) { m_CubeMaterial = material; }
	void SetTexure(ID3D11ShaderResourceView* material) { m_Texture = material; }

	void SetNumberOfInteractions(int number) { m_NumberOfIterations = number; }

	void Draw(ID3D11DeviceContext * pImmediateContext, ConstantBuffer& cb, ID3D11Buffer* Constantb);
	void Update(double delta);
	void GenerateTerrain();


	int* GetWidthParam()  { return &m_Width; }
	int* GetHeightParam() { return &m_Height; }
	int* GetDepthParam()  { return &m_Depth; }

	int* GetChunkWidthParam() { return &m_ChunkWidth; }
	int* GetChunkHeightParam() { return &m_ChunkHeight; }
	int* GetChunkDepthParam() { return &m_ChunkDepth; }

	float* GetTerrainPosX() { return &m_TerrainPosX; }
	float* GetTerrainPosY() { return &m_TerrainPosY; }
	float* GetTerrainPosZ() { return &m_TerrainPosZ; }

	float* GetCuttoffHeight() { return &m_CutoffHeight; }

	bool* GetCuttoffInverted() { return &m_IsCutoffInverted; }

	void SetWidthParam(float width) { m_Width = width; }
	void SetHeightParam(float height) { m_Height = height; }
	void SetDepthParam(float depth) { m_Depth = m_Depth; }

	void SetCutoffHeight(float height) { m_CutoffHeight = height; }

private:

	int m_NumberOfIterations = 16;

	int m_ChunkWidth = 16;
	int m_ChunkHeight = 16;
	int m_ChunkDepth = 16;

	float m_TerrainPosX = 0.0f;
	float m_TerrainPosY = 0.0f;
	float m_TerrainPosZ = 0.0f;

	float m_CutoffHeight = 0.02f;

	int m_Width = 50;
	int m_Height = 128;
	int m_Depth = 128;

	int m_XOffset = 100.0f;
	int m_YOffset = 0.0f;
	int m_ZOffset = 75.0f;

	bool m_IsCutoffInverted = false;

	vector<GameObject *> m_CubeObjects;
	GameObject* m_VoxelCube;
	Geometry m_CubeGeometry;
	Material m_CubeMaterial;
	ID3D11ShaderResourceView * m_Texture;
};
