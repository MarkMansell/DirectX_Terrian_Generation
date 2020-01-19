#include "TerrainClass.h"

TerrainClass::TerrainClass()
{
	m_VertexBuffer = 0;
	m_IndexBuffer = 0;
	m_HeightMap = 0;
	m_Texture = 0;
	m_vertexList = 0;
	m_GenType = NONE;
	m_IsWireFrame = false;
}


TerrainClass::TerrainClass(const TerrainClass& other)
{
}


TerrainClass::~TerrainClass()
{
}

bool TerrainClass::Initialize(ID3D11Device* device, char* heightmapfile, WCHAR* textureFilename)
{
	bool result;

	D3D11_RASTERIZER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_RASTERIZER_DESC));
	desc.FillMode = D3D11_FILL_WIREFRAME;
	desc.CullMode = D3D11_CULL_NONE;
	desc.DepthClipEnable = true;

	device->CreateRasterizerState(&desc, &m_WireframeRS);

	ZeroMemory(&desc, sizeof(D3D11_RASTERIZER_DESC));
	desc.FillMode = D3D11_FILL_SOLID;
	desc.CullMode = D3D11_CULL_BACK;

	device->CreateRasterizerState(&desc, &m_SolidRS);


	switch (m_GenType)
	{
	case TerrainClass::NONE:
		result = InitializeDS(device, heightmapfile, textureFilename);
		break;
	case TerrainClass::HEIGHTMAP:
		result = InitializeHeight(device, heightmapfile, textureFilename);
		break;
	case TerrainClass::VoxelTerrain:
		break;
	default:
		break;
	}

	return result;
}

bool TerrainClass::InitializeHeight(ID3D11Device* device, char* heightmapfile, WCHAR* textureFilename)
{
	bool result;
	result = LoadHeightMap(heightmapfile);
	if (!result)
	{
		return false;
	}

	NormalizeHeightMap();

	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	CalculateTextureCoordinates();

	result = LoadTexture(device, textureFilename);
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}

	return true;
}

bool TerrainClass::InitializeDS(ID3D11Device* device, char* heightmapfile, WCHAR* textureFilename)
{
	bool result;

	result = LoadTexture(device, textureFilename);
	if (!result)
	{
		return false;
	}

	result = InitializeDSBuffers(device);
	if (!result)
	{
		return false;
	}
	return true;
}

void TerrainClass::Shutdown()
{
	ReleaseTexture();
	ShutdownBuffers();
	ShutdownHeightMap();

	return;
}

void TerrainClass::ApplyHillCircle(ID3D11DeviceContext* deviceContext, int iterations, float minheight, float maxheight)
{
	
	HRESULT result = S_OK;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	deviceContext->Map(m_VertexBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource);
	vertices = reinterpret_cast<VertexType*>(mappedResource.pData);

	for (int i = 0; i < iterations; i++)
	{
		int random_x = rand() % (int)m_DSSize;
		int random_z = rand() % (int)m_DSSize;

		random_x -= m_DSSize * 0.5f;
		random_z -= m_DSSize * 0.5f;

		int random_radius = (rand() % 50) + 1;
		float disp = (rand() % 25) + 1;


		for (int i = 0; i < m_VertexCount; i++)
		{
			float x = vertices[i].position.x;
			float z = vertices[i].position.z;

			int dx = x - random_x;
			int dz = z - random_z;
			float distance = sqrtf((dx * dx) + (dz * dz));

			float pd = (distance * 2) / random_radius;

			if (fabs(pd) <= 1.0)
			{
				vertices[i].position.y += (disp / 2.0) + (cos(pd*3.14) * (disp / 2.0));
			}
		}

	}

	for (int i = 0; i < m_VertexCount; i++)
	{
		if (vertices[i].position.y < minheight)
		{
			vertices[i].position.y = minheight;
		}
		if (vertices[i].position.y > maxheight)
		{
			vertices[i].position.y = maxheight;
		}
	}

	deviceContext->Unmap(m_VertexBuffer, 0);
	vertices = 0;
}

void TerrainClass::ApplyFaultLine(ID3D11DeviceContext* deviceContext, int iterations, float height)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	deviceContext->Map(m_VertexBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource);
	vertices = reinterpret_cast<VertexType*>(mappedResource.pData);

	float maxHeight = 55.0f;
	float minHeight = -55.0f;

	for (int i = 0; i < iterations; i++)
	{
		float angle = static_cast <float> (rand());
		float angleX = sin(angle);
		float angleZ = cos(angle);
		float d = sqrt(m_DSSize * m_DSSize + m_DSSize * m_DSSize);

		float c = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * d - d / 2;

		for (int i = 0; i < m_VertexCount; i++)
		{
			float x = vertices[i].position.x;
			float z = vertices[i].position.z;

			if (angleX * x + angleZ * z - c > 0)
			{
				vertices[i].position.y += height;
				if (vertices[i].position.y > maxHeight)
				{
					vertices[i].position.y = maxHeight;
				}
			}
			else
			{
				vertices[i].position.y -= height;
				if (vertices[i].position.y < minHeight)
				{
					vertices[i].position.y = minHeight;
				}
			}
		}

	}

	deviceContext->Unmap(m_VertexBuffer, 0);
	vertices = 0;
}
void  TerrainClass::ApplyParticleDeposition(ID3D11DeviceContext* deviceContext, int numIt)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	deviceContext->Map(m_VertexBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource);
	vertices = reinterpret_cast<VertexType*>(mappedResource.pData);

	int i, dir;

	int random_x = rand() % (int)m_DSDivisions;
	int random_z = rand() % (int)m_DSDivisions;

	for (i = 0; i < numIt; i++) 
	{

		//iterationsDone++;
		dir = rand() % 4;

		if (dir == 2) {
			random_x++;
			if (random_x >= m_DSDivisions)
				random_x = 0;
		}
		else if (dir == 3) {
			random_x--;
			if (random_x == -1)
				random_x = m_DSDivisions - 1;
		}

		else if (dir == 1) {
			random_z++;
			if (random_z >= m_DSDivisions)
				random_z = 0;
		}
		else if (dir == 0) {
			random_z--;
			if (random_z == -1)
				random_z = m_DSDivisions - 1;
		}

		if (true)
		{
			ParticleDeposit(random_x, random_z);
		}
		else
			vertices[random_x * m_DSDivisions + random_z].position.y += 5.0f;
	}

	deviceContext->Unmap(m_VertexBuffer, 0);
	vertices = 0;
}

void TerrainClass::ApplySmoothing(ID3D11DeviceContext * deviceContext, float factor)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	deviceContext->Map(m_VertexBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource);
	vertices = reinterpret_cast<VertexType*>(mappedResource.pData);

	int i, j;

	for (int i = 0; i < m_DSDivisions; i++)
	{
		for (int j = 1; j < m_DSDivisions; j++)
		{
			vertices[i * m_DSDivisions + j].position.y = vertices[i * m_DSDivisions + j].position.y * (1 - factor) + vertices[i * m_DSDivisions + j - 1].position.y * factor;
		}
	}

	for (int i = 1; i < m_DSDivisions; i++)
	{
		for (int j = 0; j < m_DSDivisions; j++)
		{
			vertices[i * m_DSDivisions + j].position.y = vertices[i * m_DSDivisions + j].position.y * (1 - factor) + vertices[(i - 1) * m_DSDivisions + j].position.y * factor;
		}
	}

	for (int i = 0; i < m_DSDivisions; i++)
	{
		for (int j = m_DSDivisions - 1; j > -1; j--)
		{
			vertices[i * m_DSDivisions + j].position.y = vertices[i * m_DSDivisions + j].position.y  * (1 - factor) + vertices[i * m_DSDivisions + j + 1].position.y  * factor;
		}
	}

	for (i = m_DSDivisions - 2; i < -1; i--)
	{
		for (j = 0;j < m_DSDivisions; j++)
		{
			vertices[i * m_DSDivisions + j].position.y = vertices[i * m_DSDivisions + j].position.y  * (1 - factor) + vertices[(i + 1)*m_DSDivisions + j].position.y  * factor;
		}
	}

	deviceContext->Unmap(m_VertexBuffer, 0);
	vertices = 0;
}

void TerrainClass::ParticleDeposit(int x, int z)
{
	int j, k, kk, jj, flag;

	flag = 0;
	for (k = -1;k < 2;k++)
	{
		for (j = -1;j < 2;j++)
		{
			if (k != 0 && j != 0 && x + k > -1 && x + k < m_DSDivisions && z + j>-1 && z + j < m_DSDivisions)
			{
				if (vertices[(x + k) * m_DSDivisions + (z + j)].position.y < vertices[x * m_DSDivisions + z].position.y)
				{
					flag = 1;
					kk = k;
					jj = j;
				}
			}
		}
	}

	if (!flag)
		vertices[x * m_DSDivisions + z].position.y += 5.0f;
	else
		ParticleDeposit(x + kk, z + jj);
}


void TerrainClass::ApplyDiamondSquare(ID3D11DeviceContext* deviceContext, float height)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	deviceContext->Map(m_VertexBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource);
	vertices = reinterpret_cast<VertexType*>(mappedResource.pData);

	vertices[0].position.y = RandomFloat(-height, height);
	vertices[m_DSDivisions].position.y = RandomFloat(-height, height);
	vertices[m_VertexCount - 1].position.y = RandomFloat(-height, height);
	vertices[m_VertexCount - (m_DSDivisions + 1)].position.y = RandomFloat(-height, height);

	int interations = (int)(log(m_DSDivisions) / log(2));
	int numSqaures = 1;

	int squareSize = m_DSDivisions;

	for (int i = 0; i < interations; i++)
	{
		int row = 0;
		for (int j = 0; j < numSqaures; j++)
		{
			int col = 0;
			for (int k = 0; k < numSqaures; k++)
			{
				DiamondSquare(row, col, squareSize, height);
				col += squareSize;
			}
			row += squareSize;
		}
		numSqaures *= 2;
		squareSize /= 2;
		height *= 0.5f;
	}

	deviceContext->Unmap(m_VertexBuffer, 0);
	vertices = 0;
}

void TerrainClass::ResetTerrain(ID3D11DeviceContext* deviceContext)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	deviceContext->Map(m_VertexBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource);
	vertices = reinterpret_cast<VertexType*>(mappedResource.pData);

	for (int i = 0; i < m_VertexCount; i++)
	{
		vertices[i].position.y = 0.0f;
	}
	
	deviceContext->Unmap(m_VertexBuffer, 0);
	vertices = 0;
}

void TerrainClass::Render(ID3D11DeviceContext* deviceContext)
{
	unsigned int stride;
	unsigned int offset;


	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	deviceContext->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	if(m_IsWireFrame)
		deviceContext->RSSetState(m_WireframeRS);
	else
		deviceContext->RSSetState(m_SolidRS);

	return;
}

int TerrainClass::GetIndexCount()
{
	return m_IndexCount;
}

int TerrainClass::GetVertexCount()
{
	return m_VertexCount;
}

bool TerrainClass::InitializeDSBuffers(ID3D11Device* device)
{
	bool result;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;

	m_VertexCount = (m_DSDivisions + 1) * (m_DSDivisions + 1);

	float halfSize = m_DSSize * 0.5f;
	float m_DSDivisionSize = m_DSSize / m_DSDivisions;

	vertices = new VertexType[m_VertexCount];
	if (!vertices)
		return false;
	m_IndexCount = m_DSDivisions * m_DSDivisions * 6;

	indices = new unsigned long[m_IndexCount];
	if (!indices)
		return false;

	int offset = 0;

	for (int i = 0; i <= m_DSDivisions; i++)
	{
		for (int j = 0; j <= m_DSDivisions; j++)
		{
			vertices[i * (m_DSDivisions + 1) + j].position = XMFLOAT3(-halfSize + j * m_DSDivisionSize, 0.0f, halfSize - i * m_DSDivisionSize);
			vertices[i * (m_DSDivisions + 1) + j].texture = XMFLOAT2((float)i/m_DSDivisions, (float)j/m_DSDivisions);

			if (i < m_DSDivisions && j < m_DSDivisions)
			{
				int topleft = i * (m_DSDivisions + 1) + j;
				int botleft = (i + 1) * (m_DSDivisions + 1) + j;

				indices[offset] = topleft;
				indices[offset + 1] = topleft + 1;
				indices[offset + 2] = botleft + 1;

				indices[offset + 3] = topleft;
				indices[offset + 4] = botleft + 1;
				indices[offset + 5] = botleft;

				offset += 6;
			}
		}
	}	

	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_VertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_VertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_IndexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_IndexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	delete[] vertices;
	vertices = 0;

	//delete[] indices;
	//indices = 0;
}

void TerrainClass::DiamondSquare(int row, int col, int size, float offset)
{
	int halfsize = (int(size * 0.5f));
	int topleft = (row * (m_DSDivisions + 1)) + col;
	int botleft = ((row + size) * (m_DSDivisions + 1)) + col;
	int mid = ((int)(row + halfsize) * (m_DSDivisions + 1)) + (int)(col + halfsize);

	vertices[mid].position.y = (vertices[topleft].position.y + vertices[topleft + size].position.y + vertices[botleft].position.y + vertices[botleft + size].position.y) * 0.25f + RandomFloat(-offset, offset);

	// Square Part
	vertices[topleft + halfsize].position.y = (vertices[topleft].position.y + vertices[topleft + size].position.y + vertices[mid].position.y) / 3 + RandomFloat(-offset, offset);
	vertices[mid - halfsize].position.y = (vertices[topleft].position.y + vertices[botleft].position.y + vertices[mid].position.y) / 3 + RandomFloat(-offset, offset);
	vertices[mid + halfsize].position.y = (vertices[topleft + size].position.y + vertices[botleft + size].position.y + vertices[mid].position.y) / 3 + RandomFloat(-offset, offset);
	vertices[botleft + halfsize].position.y = (vertices[botleft].position.y + vertices[botleft + size].position.y + vertices[mid].position.y) / 3 + RandomFloat(-offset, offset);
}

float TerrainClass::RandomFloat(float low, float high)
{
	float temp;

	if (low > high)
	{
		temp = low;
		low = high;
		high = temp;
	}

	temp = (rand() / (static_cast<float>(RAND_MAX) + 1.0))
		* (high - low) + low;
	return temp;
}

bool TerrainClass::InitializeBuffers(ID3D11Device* device)
{
	VertexType* vertices;
	unsigned long* indices;
	int index, i, j;
	float positionX, positionZ;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;
	int index1, index2, index3, index4;
	float tu, tv;

	// Vertex count for grid
	m_VertexCount = (m_TerrainWidth - 1) * (m_TerrainHeight - 1) * 6;

	m_vertexList = new VectorType[m_VertexCount];
	m_IndexCount = m_VertexCount;

	vertices = new VertexType[m_VertexCount];
	if (!vertices)
		return false;


	indices = new unsigned long[m_IndexCount];
	if (!indices)
		return false;
	
	// Initialize the index to the vertex buffer.
	index = 0;

	for (j = 0; j < (m_TerrainHeight - 1); j++)
	{
		for (i = 0; i < (m_TerrainWidth - 1); i++)
		{
			index1 = (m_TerrainHeight * j) + i;          // Bottom left.
			index2 = (m_TerrainHeight * j) + (i + 1);      // Bottom right.
			index3 = (m_TerrainHeight * (j + 1)) + i;      // Upper left.
			index4 = (m_TerrainHeight * (j + 1)) + (i + 1);  // Upper right.

															 // Upper left.
			tv = m_HeightMap[index3].tv;
						
			if (tv == 1.0f) { tv = 0.0f; }

			vertices[index].position = XMFLOAT3(m_HeightMap[index3].x, m_HeightMap[index3].y, m_HeightMap[index3].z);
			vertices[index].texture = XMFLOAT2(m_HeightMap[index3].tu, tv);
			vertices[index].normal = XMFLOAT3(m_HeightMap[index3].nx, m_HeightMap[index3].ny, m_HeightMap[index3].nz);

			m_vertexList[index].x = vertices[index].position.x;
			m_vertexList[index].y = vertices[index].position.y;
			m_vertexList[index].z = vertices[index].position.z;

			indices[index] = index;
			index++;
						
			tu = m_HeightMap[index4].tu;
			tv = m_HeightMap[index4].tv;
						
			if (tu == 0.0f) { tu = 1.0f; }
			if (tv == 1.0f) { tv = 0.0f; }

			vertices[index].position = XMFLOAT3(m_HeightMap[index4].x, m_HeightMap[index4].y, m_HeightMap[index4].z);
			vertices[index].texture = XMFLOAT2(tu, tv);
			vertices[index].normal = XMFLOAT3(m_HeightMap[index4].nx, m_HeightMap[index4].ny, m_HeightMap[index4].nz);
			
			m_vertexList[index].x = vertices[index].position.x;
			m_vertexList[index].y = vertices[index].position.y;
			m_vertexList[index].z = vertices[index].position.z;

			indices[index] = index;
			index++;

			// Bottom left.
			vertices[index].position = XMFLOAT3(m_HeightMap[index1].x, m_HeightMap[index1].y, m_HeightMap[index1].z);
			vertices[index].texture = XMFLOAT2(m_HeightMap[index1].tu, m_HeightMap[index1].tv);
			vertices[index].normal = XMFLOAT3(m_HeightMap[index1].nx, m_HeightMap[index1].ny, m_HeightMap[index1].nz);

			m_vertexList[index].x = vertices[index].position.x;
			m_vertexList[index].y = vertices[index].position.y;
			m_vertexList[index].z = vertices[index].position.z;

			indices[index] = index;
			index++;

			// Bottom left.
			vertices[index].position = XMFLOAT3(m_HeightMap[index1].x, m_HeightMap[index1].y, m_HeightMap[index1].z);
			vertices[index].texture = XMFLOAT2(m_HeightMap[index1].tu, m_HeightMap[index1].tv);
			vertices[index].normal = XMFLOAT3(m_HeightMap[index1].nx, m_HeightMap[index1].ny, m_HeightMap[index1].nz);

			m_vertexList[index].x = vertices[index].position.x;
			m_vertexList[index].y = vertices[index].position.y;
			m_vertexList[index].z = vertices[index].position.z;

			indices[index] = index;
			index++;

			// Upper right.
			tu = m_HeightMap[index4].tu;
			tv = m_HeightMap[index4].tv;

			// Modify the texture coordinates to cover the top and right edge.
			if (tu == 0.0f) { tu = 1.0f; }
			if (tv == 1.0f) { tv = 0.0f; }

			vertices[index].position = XMFLOAT3(m_HeightMap[index4].x, m_HeightMap[index4].y, m_HeightMap[index4].z);
			vertices[index].texture = XMFLOAT2(tu, tv);
			vertices[index].normal = XMFLOAT3(m_HeightMap[index4].nx, m_HeightMap[index4].ny, m_HeightMap[index4].nz);

			m_vertexList[index].x = vertices[index].position.x;
			m_vertexList[index].y = vertices[index].position.y;
			m_vertexList[index].z = vertices[index].position.z;

			indices[index] = index;
			index++;

			// Bottom right.
			tu = m_HeightMap[index2].tu;

			// Modify the texture coordinates to cover the right edge.
			if (tu == 0.0f) { tu = 1.0f; }

			vertices[index].position = XMFLOAT3(m_HeightMap[index2].x, m_HeightMap[index2].y, m_HeightMap[index2].z);
			vertices[index].texture = XMFLOAT2(tu, m_HeightMap[index2].tv);
			vertices[index].normal = XMFLOAT3(m_HeightMap[index2].nx, m_HeightMap[index2].ny, m_HeightMap[index2].nz);

			m_vertexList[index].x = vertices[index].position.x;
			m_vertexList[index].y = vertices[index].position.y;
			m_vertexList[index].z = vertices[index].position.z;

			indices[index] = index;
			index++;
		}
	}

	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_VertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_VertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_IndexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_IndexBuffer);
	if (FAILED(result))
	{
		return false;
	}
	
	delete[] vertices;
	vertices = 0;

	delete[] indices;
	indices = 0;

	return true;
}

bool TerrainClass::GetHeightAtPosition(float inputX, float inputZ, float& height)
{

	if (!vertices)
		return false;

	int i;
	int cellId;
	int index;

	float vertex1[3];
	float vertex2[3];
	float vertex3[3];
	bool foundHeight;

	float maxWidth;
	float maxHeight;
	float maxDepth;
	float minWidth;
	float minHeight;
	float minDepth;

	// If this is the right cell then check all the triangles in this cell to see what the height of the triangle at this position is.
	for (i = 0; i < (m_IndexCount / 3 ); i++)
	{
		index = i * 3;

		vertex1[0] = vertices[indices[index]].position.x;
		vertex1[1] = vertices[indices[index]].position.y;
		vertex1[2] = vertices[indices[index]].position.z;
		index++;					
									
		vertex2[0] = vertices[indices[index]].position.x;
		vertex2[1] = vertices[indices[index]].position.y;
		vertex2[2] = vertices[indices[index]].position.z;
		index++;				
									
		vertex3[0] = vertices[indices[index]].position.x;
		vertex3[1] = vertices[indices[index]].position.y;
		vertex3[2] = vertices[indices[index]].position.z;

		// Check to see if this is the polygon we are looking for.
		foundHeight = CheckHeightOfTriangle(inputX, inputZ, height, vertex1, vertex2, vertex3);
		if (foundHeight)
		{
			return true;
		}
	}

	return false;
}

bool TerrainClass::CheckHeightOfTriangle(float x, float z, float& height, float v0[3], float v1[3], float v2[3])
{
	float startVector[3], directionVector[3], edge1[3], edge2[3], normal[3];
	float Q[3], e1[3], e2[3], e3[3], edgeNormal[3], temp[3];
	float magnitude, D, denominator, numerator, t, determinant;


	// Starting position of the ray that is being cast.
	startVector[0] = x;
	startVector[1] = 0.0f;
	startVector[2] = z;

	// The direction the ray is being cast.
	directionVector[0] = 0.0f;
	directionVector[1] = -1.0f;
	directionVector[2] = 0.0f;

	// Calculate the two edges from the three points given.
	edge1[0] = v1[0] - v0[0];
	edge1[1] = v1[1] - v0[1];
	edge1[2] = v1[2] - v0[2];

	edge2[0] = v2[0] - v0[0];
	edge2[1] = v2[1] - v0[1];
	edge2[2] = v2[2] - v0[2];

	// Calculate the normal of the triangle from the two edges.
	normal[0] = (edge1[1] * edge2[2]) - (edge1[2] * edge2[1]);
	normal[1] = (edge1[2] * edge2[0]) - (edge1[0] * edge2[2]);
	normal[2] = (edge1[0] * edge2[1]) - (edge1[1] * edge2[0]);

	magnitude = (float)sqrt((normal[0] * normal[0]) + (normal[1] * normal[1]) + (normal[2] * normal[2]));
	normal[0] = normal[0] / magnitude;
	normal[1] = normal[1] / magnitude;
	normal[2] = normal[2] / magnitude;

	// Find the distance from the origin to the plane.
	D = ((-normal[0] * v0[0]) + (-normal[1] * v0[1]) + (-normal[2] * v0[2]));

	// Get the denominator of the equation.
	denominator = ((normal[0] * directionVector[0]) + (normal[1] * directionVector[1]) + (normal[2] * directionVector[2]));

	// Make sure the result doesn't get too close to zero to prevent divide by zero.
	if (fabs(denominator) < 0.0001f)
	{
		return false;
	}

	// Get the numerator of the equation.
	numerator = -1.0f * (((normal[0] * startVector[0]) + (normal[1] * startVector[1]) + (normal[2] * startVector[2])) + D);

	// Calculate where we intersect the triangle.
	t = numerator / denominator;

	// Find the intersection vector.
	Q[0] = startVector[0] + (directionVector[0] * t);
	Q[1] = startVector[1] + (directionVector[1] * t);
	Q[2] = startVector[2] + (directionVector[2] * t);

	// Find the three edges of the triangle.
	e1[0] = v1[0] - v0[0];
	e1[1] = v1[1] - v0[1];
	e1[2] = v1[2] - v0[2];

	e2[0] = v2[0] - v1[0];
	e2[1] = v2[1] - v1[1];
	e2[2] = v2[2] - v1[2];

	e3[0] = v0[0] - v2[0];
	e3[1] = v0[1] - v2[1];
	e3[2] = v0[2] - v2[2];

	// Calculate the normal for the first edge.
	edgeNormal[0] = (e1[1] * normal[2]) - (e1[2] * normal[1]);
	edgeNormal[1] = (e1[2] * normal[0]) - (e1[0] * normal[2]);
	edgeNormal[2] = (e1[0] * normal[1]) - (e1[1] * normal[0]);

	// Calculate the determinant to see if it is on the inside, outside, or directly on the edge.
	temp[0] = Q[0] - v0[0];
	temp[1] = Q[1] - v0[1];
	temp[2] = Q[2] - v0[2];

	determinant = ((edgeNormal[0] * temp[0]) + (edgeNormal[1] * temp[1]) + (edgeNormal[2] * temp[2]));

	// Check if it is outside.
	if (determinant > 0.001f)
	{
		return false;
	}

	// Calculate the normal for the second edge.
	edgeNormal[0] = (e2[1] * normal[2]) - (e2[2] * normal[1]);
	edgeNormal[1] = (e2[2] * normal[0]) - (e2[0] * normal[2]);
	edgeNormal[2] = (e2[0] * normal[1]) - (e2[1] * normal[0]);

	// Calculate the determinant to see if it is on the inside, outside, or directly on the edge.
	temp[0] = Q[0] - v1[0];
	temp[1] = Q[1] - v1[1];
	temp[2] = Q[2] - v1[2];

	determinant = ((edgeNormal[0] * temp[0]) + (edgeNormal[1] * temp[1]) + (edgeNormal[2] * temp[2]));

	// Check if it is outside.
	if (determinant > 0.001f)
	{
		return false;
	}

	// Calculate the normal for the third edge.
	edgeNormal[0] = (e3[1] * normal[2]) - (e3[2] * normal[1]);
	edgeNormal[1] = (e3[2] * normal[0]) - (e3[0] * normal[2]);
	edgeNormal[2] = (e3[0] * normal[1]) - (e3[1] * normal[0]);

	// Calculate the determinant to see if it is on the inside, outside, or directly on the edge.
	temp[0] = Q[0] - v2[0];
	temp[1] = Q[1] - v2[1];
	temp[2] = Q[2] - v2[2];

	determinant = ((edgeNormal[0] * temp[0]) + (edgeNormal[1] * temp[1]) + (edgeNormal[2] * temp[2]));

	// Check if it is outside.
	if (determinant > 0.001f)
	{
		return false;
	}

	// Now we have our height.
	height = Q[1];

	return true;
}


bool TerrainClass::LoadTexture(ID3D11Device* device, WCHAR* filename)
{
	HRESULT result = S_OK;
	result = CreateDDSTextureFromFile(device, filename, nullptr, &m_Texture);
	if (FAILED(result))
	{
		return false;
	}
	return true;
}
void TerrainClass::ReleaseTexture()
{
	// Release the texture object.
	if (m_Texture)
	{
		m_Texture->Release();
	}

	return;
}

bool TerrainClass::LoadHeightMap(char* filename)
{
	FILE* filePtr;
	int error;
	unsigned int count;
	BITMAPFILEHEADER bitmapFileHeader;
	BITMAPINFOHEADER bitmapInfoHeader;
	int imageSize, i, j, k, index;
	unsigned char* bitmapImage;
	unsigned char height;

	// Open the height map file in binary.
	error = fopen_s(&filePtr, filename, "rb");
	if (error != 0)
	{
		return false;
	}

	// Read in the file header.
	count = fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);
	if (count != 1)
	{
		return false;
	}

	// Read in the bitmap info header.
	count = fread(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);
	if (count != 1)
	{
		return false;
	}

	// Save the dimensions of the terrain.
	m_TerrainWidth = bitmapInfoHeader.biWidth;
	m_TerrainHeight = bitmapInfoHeader.biHeight;

	// Calculate the size of the bitmap image data.
	imageSize = m_TerrainWidth * m_TerrainHeight * 3;

	// Allocate memory for the bitmap image data.
	bitmapImage = new unsigned char[imageSize];
	if (!bitmapImage)
	{
		return false;
	}

	// Move to the beginning of the bitmap data.
	fseek(filePtr, bitmapFileHeader.bfOffBits, SEEK_SET);

	// Read in the bitmap image data.
	count = fread(bitmapImage, 1, imageSize, filePtr);
	if (count != imageSize)
	{
		return false;
	}

	// Close the file.
	error = fclose(filePtr);
	if (error != 0)
	{
		return false;
	}

	// Create the structure to hold the height map data.
	m_HeightMap = new HeightMapType[m_TerrainWidth * m_TerrainHeight];
	if (!m_HeightMap)
	{
		return false;
	}

	// Initialize the position in the image data buffer.
	k = 0;

	// Read the image data into the height map.
	for (j = 0; j < m_TerrainHeight; j++)
	{
		for (i = 0; i < m_TerrainWidth; i++)
		{
			height = bitmapImage[k];

			index = (m_TerrainHeight * j) + i;

			m_HeightMap[index].x = (float)i;
			m_HeightMap[index].y = (float)height;
			m_HeightMap[index].z = (float)j;

			k += 3;
		}
	}

	// Release the bitmap image data.
	delete[] bitmapImage;
	bitmapImage = 0;

	return true;
}

void TerrainClass::ShutdownBuffers()
{
	if (m_IndexBuffer)
	{
		m_IndexBuffer->Release();
		m_IndexBuffer = 0;
	}

	if (m_VertexBuffer)
	{
		m_VertexBuffer->Release();
		m_VertexBuffer = 0;
	}

	if (m_WireframeRS)
	{
		m_WireframeRS->Release();
		m_WireframeRS = 0;
	}

	if (m_SolidRS)
	{
		m_SolidRS->Release();
		m_SolidRS = 0;
	}

	return;
}

void TerrainClass::NormalizeHeightMap()
{
	int i, j;

	for (j = 0; j < m_TerrainHeight; j++)
	{
		for (i = 0; i < m_TerrainWidth; i++)
		{
			m_HeightMap[(m_TerrainHeight * j) + i].y /= 15.0f;
		}
	}

	return;
}

void TerrainClass::ShutdownHeightMap()
{
	if (m_HeightMap)
	{
		delete[] m_HeightMap;
		m_HeightMap = 0;
	}

	return;
}

bool TerrainClass::CalculateNormals()
{
	int i, j, index1, index2, index3, index, count;
	float vertex1[3], vertex2[3], vertex3[3], vector1[3], vector2[3], sum[3], length;
	VectorType* normals;


	// Create a temporary array to hold the un-normalized normal vectors.
	normals = new VectorType[(m_TerrainHeight - 1) * (m_TerrainWidth - 1)];
	if (!normals)
	{
		return false;
	}

	// Go through all the faces in the mesh and calculate their normals.
	for (j = 0; j<(m_TerrainHeight - 1); j++)
	{
		for (i = 0; i<(m_TerrainWidth - 1); i++)
		{
			index1 = (j * m_TerrainHeight) + i;
			index2 = (j * m_TerrainHeight) + (i + 1);
			index3 = ((j + 1) * m_TerrainHeight) + i;

			// Get three vertices from the face.
			vertex1[0] = m_HeightMap[index1].x;
			vertex1[1] = m_HeightMap[index1].y;
			vertex1[2] = m_HeightMap[index1].z;

			vertex2[0] = m_HeightMap[index2].x;
			vertex2[1] = m_HeightMap[index2].y;
			vertex2[2] = m_HeightMap[index2].z;

			vertex3[0] = m_HeightMap[index3].x;
			vertex3[1] = m_HeightMap[index3].y;
			vertex3[2] = m_HeightMap[index3].z;

			// Calculate the two vectors for this face.
			vector1[0] = vertex1[0] - vertex3[0];
			vector1[1] = vertex1[1] - vertex3[1];
			vector1[2] = vertex1[2] - vertex3[2];
			vector2[0] = vertex3[0] - vertex2[0];
			vector2[1] = vertex3[1] - vertex2[1];
			vector2[2] = vertex3[2] - vertex2[2];

			index = (j * (m_TerrainHeight - 1)) + i;

			// Calculate the cross product of those two vectors to get the un-normalized value for this face normal.
			normals[index].x = (vector1[1] * vector2[2]) - (vector1[2] * vector2[1]);
			normals[index].y = (vector1[2] * vector2[0]) - (vector1[0] * vector2[2]);
			normals[index].z = (vector1[0] * vector2[1]) - (vector1[1] * vector2[0]);
		}
	}

	// Now go through all the vertices and take an average of each face normal 	
	// that the vertex touches to get the averaged normal for that vertex.
	for (j = 0; j<m_TerrainHeight; j++)
	{
		for (i = 0; i<m_TerrainWidth; i++)
		{
			// Initialize the sum.
			sum[0] = 0.0f;
			sum[1] = 0.0f;
			sum[2] = 0.0f;

			// Initialize the count.
			count = 0;

			// Bottom left face.
			if (((i - 1) >= 0) && ((j - 1) >= 0))
			{
				index = ((j - 1) * (m_TerrainHeight - 1)) + (i - 1);

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Bottom right face.
			if ((i < (m_TerrainWidth - 1)) && ((j - 1) >= 0))
			{
				index = ((j - 1) * (m_TerrainHeight - 1)) + i;

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Upper left face.
			if (((i - 1) >= 0) && (j < (m_TerrainHeight - 1)))
			{
				index = (j * (m_TerrainHeight - 1)) + (i - 1);

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Upper right face.
			if ((i < (m_TerrainWidth - 1)) && (j < (m_TerrainHeight - 1)))
			{
				index = (j * (m_TerrainHeight - 1)) + i;

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Take the average of the faces touching this vertex.
			sum[0] = (sum[0] / (float)count);
			sum[1] = (sum[1] / (float)count);
			sum[2] = (sum[2] / (float)count);

			// Calculate the length of this normal.
			length = sqrt((sum[0] * sum[0]) + (sum[1] * sum[1]) + (sum[2] * sum[2]));

			// Get an index to the vertex location in the height map array.
			index = (j * m_TerrainHeight) + i;

			// Normalize the final shared normal for this vertex and store it in the height map array.
			m_HeightMap[index].nx = (sum[0] / length);
			m_HeightMap[index].ny = (sum[1] / length);
			m_HeightMap[index].nz = (sum[2] / length);
		}
	}

	// Release the temporary normals.
	delete[] normals;
	normals = 0;

	return true;
}

void TerrainClass::CalculateTextureCoordinates()
{
	int incrementCount, i, j, tuCount, tvCount;
	float incrementValue, tuCoordinate, tvCoordinate;


	// Calculate how much to increment the texture coordinates by.
	incrementValue = (float)TEXTURE_REPEAT / (float)m_TerrainWidth;

	// Calculate how many times to repeat the texture.
	incrementCount = m_TerrainWidth / TEXTURE_REPEAT;

	// Initialize the tu and tv coordinate values.
	tuCoordinate = 0.0f;
	tvCoordinate = 1.0f;

	// Initialize the tu and tv coordinate indexes.
	tuCount = 0;
	tvCount = 0;

	// Loop through the entire height map and calculate the tu and tv texture coordinates for each vertex.
	for (j = 0; j < m_TerrainHeight; j++)
	{
		for (i = 0; i < m_TerrainWidth; i++)
		{
			// Store the texture coordinate in the height map.
			m_HeightMap[(m_TerrainHeight * j) + i].tu = tuCoordinate;
			m_HeightMap[(m_TerrainHeight * j) + i].tv = tvCoordinate;

			// Increment the tu texture coordinate by the increment value and increment the index by one.
			tuCoordinate += incrementValue;
			tuCount++;

			// Check if at the far right end of the texture and if so then start at the beginning again.
			if (tuCount == incrementCount)
			{
				tuCoordinate = 0.0f;
				tuCount = 0;
			}
		}

		// Increment the tv texture coordinate by the increment value and increment the index by one.
		tvCoordinate -= incrementValue;
		tvCount++;

		// Check if at the top of the texture and if so then start at the bottom again.
		if (tvCount == incrementCount)
		{
			tvCoordinate = 1.0f;
			tvCount = 0;
		}
	}

	return;
}

