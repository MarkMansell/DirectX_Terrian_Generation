#include "VoxelTerrain.h"

VoxelTerrain::VoxelTerrain()
{

}

VoxelTerrain::~VoxelTerrain()
{
}

void VoxelTerrain::Update(double delta)
{
	for (auto gameObject : m_CubeObjects)
	{
		gameObject->Update(delta);
	}
	m_VoxelCube->Update(delta);
}


void VoxelTerrain::Draw(ID3D11DeviceContext * pImmediateContext, ConstantBuffer& constantbuffer, ID3D11Buffer* Constantb)
{
	ConstantBuffer cb = constantbuffer;
	unsigned int seed = 237;


	module::RidgedMulti mountainTerrain;

	module::Billow baseFlatTerrain;
	baseFlatTerrain.SetFrequency(2.0);

	module::ScaleBias flatTerrain;
	flatTerrain.SetSourceModule(0, baseFlatTerrain);
	flatTerrain.SetScale(0.125);
	flatTerrain.SetBias(-0.75);

	module::Perlin terrainType;
	terrainType.SetFrequency(0.5);
	terrainType.SetPersistence(0.25);

	module::Select terrainSelector;
	terrainSelector.SetSourceModule(0, flatTerrain);
	terrainSelector.SetSourceModule(1, mountainTerrain);
	terrainSelector.SetControlModule(terrainType);
	terrainSelector.SetBounds(0.0, 1000.0);
	terrainSelector.SetEdgeFalloff(0.125);

	module::Turbulence finalTerrain;
	finalTerrain.SetSourceModule(0, terrainSelector);
	finalTerrain.SetFrequency(4.0);
	finalTerrain.SetPower(0.125);

	for (int i = 0; i < m_ChunkWidth; i++)
	{
		for (int j = 0; j < m_ChunkHeight; j++)
		{
			for (int k = 0; k < m_ChunkDepth; k++)
			{
				double x = (double)i / ((double)m_Width);
				double y = (double)j / ((double)m_Height);
				double z = (double)k / ((double)m_Depth);

				x += m_TerrainPosX;
				y += m_TerrainPosY;
				z += m_TerrainPosZ;

				double noise = finalTerrain.GetValue(x, y, z);

				if (m_IsCutoffInverted)
				{
					if (noise < m_CutoffHeight || noise < 0)
						continue;
				}
				else
				{
					if (noise > m_CutoffHeight || noise < 0)
						continue;
				}

				Material material = m_VoxelCube->GetMaterial();
				m_VoxelCube->SetPosition((i * 1.0f) + m_XOffset, (j * 1.0f) + m_YOffset, (k * 1.0f) + m_ZOffset);
				m_VoxelCube->Update(0.0f);
				cb.World = DirectX::XMMatrixTranspose(m_VoxelCube->GetWorldMatrix());
				ID3D11ShaderResourceView * textureRV = m_VoxelCube->GetTextureRV();
				pImmediateContext->PSSetShaderResources(0, 1, &textureRV);

				pImmediateContext->UpdateSubresource(Constantb, 0, nullptr, &cb, 0, 0);

				m_VoxelCube->Draw(pImmediateContext);
			}
		}
	}






	for (int i = 0; i < m_CubeObjects.size(); i++)
	{
		// Get render material
		Material material = m_CubeObjects[i]->GetMaterial();

		//// Copy material to shader
		//cb->surface.AmbientMtrl = material.ambient;
		//cb->surface.DiffuseMtrl = material.diffuse;
		//cb->surface.SpecularMtrl = material.specular;

		// Set world matrix
		cb.World = DirectX::XMMatrixTranspose(m_CubeObjects[i]->GetWorldMatrix());

		if (m_CubeObjects[i]->HasTexture())
		{
			ID3D11ShaderResourceView * textureRV = m_CubeObjects[i]->GetTextureRV();
			pImmediateContext->PSSetShaderResources(0, 1, &textureRV);
		}
		else
		{
			//cb->HasTexture = 0.0f;
		}

		pImmediateContext->UpdateSubresource(Constantb, 0, nullptr, &cb, 0, 0);

		m_CubeObjects[i]->Draw(pImmediateContext);

	}
}

void VoxelTerrain::GenerateTerrain()
{
	GameObject * gameObject = new GameObject("Voxel ", m_CubeGeometry, m_CubeMaterial);
	gameObject->SetPosition(0.0f, 0.0f, 0.0f);
	gameObject->SetScale(1.0f, 1.0f, 1.0f);
	gameObject->SetRotation(0.0f, 0.0f, 0.0f);
	gameObject->SetTextureRV(m_Texture);

	m_VoxelCube = gameObject;
}
