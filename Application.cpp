#include "Application.h"

// Data
static ID3D11Device*            _pd3dDevice = NULL;
static ID3D11DeviceContext*     _pImmediateContext = NULL;
static IDXGISwapChain*          _pSwapChain = NULL;
static ID3D11RenderTargetView*  _pRenderTargetView = NULL;

void CreateRenderTarget()
{
	DXGI_SWAP_CHAIN_DESC sd;
	_pSwapChain->GetDesc(&sd);

	// Create the render target
	ID3D11Texture2D* pBackBuffer;
	D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
	ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
	render_target_view_desc.Format = sd.BufferDesc.Format;
	render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	_pd3dDevice->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &_pRenderTargetView);
	_pImmediateContext->OMSetRenderTargets(1, &_pRenderTargetView, NULL);
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	if (_pRenderTargetView) { _pRenderTargetView->Release(); _pRenderTargetView = NULL; }
}

extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;


	if (ImGui_ImplDX11_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

bool Application::HandleKeyboard(MSG msg)
{
	switch (msg.wParam)
	{
	case VK_ESCAPE:
		PostQuitMessage(WM_QUIT);
		return true;
		break;
	}

	return false;
}

Application::Application()
{
	_hInst = nullptr;
	_hWnd = nullptr;
	_driverType = D3D_DRIVER_TYPE_NULL;
	_featureLevel = D3D_FEATURE_LEVEL_11_0;
	_pd3dDevice = nullptr;
	_pImmediateContext = nullptr;
	_pSwapChain = nullptr;
	_pRenderTargetView = nullptr;
	_pVertexShader = nullptr;
	_pPixelShader = nullptr;
	_pVertexLayout = nullptr;
	_pVertexBuffer = nullptr;
	_pIndexBuffer = nullptr;
	_pConstantBuffer = nullptr;
	_pTerrainClass = nullptr;
	_pHeightTerrainClass = nullptr;
	_pVoxelTerrainClass = nullptr;
	_pTerrainColour = nullptr;

	DSLessEqual = nullptr;
	RSCullNone = nullptr;

	_isTerrainWireframe = false;
	_iTerrainState = 0;
}

Application::~Application()
{
	Cleanup();
}

HRESULT Application::Initialise(HINSTANCE hInstance, int nCmdShow)
{
    if (FAILED(InitWindow(hInstance, nCmdShow)))
	{
        return E_FAIL;
	}

    RECT rc;
    GetClientRect(_hWnd, &rc);
    _WindowWidth = rc.right - rc.left;
    _WindowHeight = rc.bottom - rc.top;

    if (FAILED(InitDevice()))
    {
        Cleanup();
        return E_FAIL;
    }
	ImGui_ImplDX11_Init(_hWnd, _pd3dDevice, _pImmediateContext);

	CreateDDSTextureFromFile(_pd3dDevice, L"Resources\\stone.dds", nullptr, &_pTextureRV);
	CreateDDSTextureFromFile(_pd3dDevice, L"Resources\\floor.dds", nullptr, &_pGroundTextureRV);
	
	_frame = 0.0f;
	isReversingFrame = false;

    // Setup Camera
	_camera = new Camera();
	_camera->SetPosition(XMFLOAT3(141.35f, 35.13f, 21.61f));
	_camera->LookAt(_camera->GetPosition(), XMFLOAT3(100.0f, 4.5f, 75.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	_camera->SetLens(0.25f * XM_PI, (float)_renderWidth / (float)_renderHeight, 0.01f, 1000.0f);

	// Setup the scene's light
	basicLight.AmbientLight = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	basicLight.DiffuseLight = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	basicLight.SpecularLight = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	basicLight.SpecularPower = 20.0f;
	basicLight.LightVecW = XMFLOAT3(0.0f, 1.0f, -1.0f);

	Geometry cubeGeometry;
	cubeGeometry.indexBuffer = _pIndexBuffer;
	cubeGeometry.vertexBuffer = _pVertexBuffer;
	cubeGeometry.numberOfIndices = 36;
	cubeGeometry.vertexBufferOffset = 0;
	cubeGeometry.vertexBufferStride = sizeof(SimpleVertex);

	Geometry planeGeometry;
	planeGeometry.indexBuffer = _pPlaneIndexBuffer;
	planeGeometry.vertexBuffer = _pPlaneVertexBuffer;
	planeGeometry.numberOfIndices = 6;
	planeGeometry.vertexBufferOffset = 0;
	planeGeometry.vertexBufferStride = sizeof(SimpleVertex);

	Material shinyMaterial;
	shinyMaterial.ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	shinyMaterial.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	shinyMaterial.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	shinyMaterial.specularPower = 10.0f;

	Material noSpecMaterial;
	noSpecMaterial.ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	noSpecMaterial.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	noSpecMaterial.specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	noSpecMaterial.specularPower = 0.0f;

	_pVoxelTerrainClass->SetCubeGeometry(cubeGeometry);
	_pVoxelTerrainClass->SetMaterial(noSpecMaterial); // Re-add
	_pVoxelTerrainClass->SetTexure(_pTextureRV);
	_pVoxelTerrainClass->GenerateTerrain();

	
	GameObject * gameObject = new GameObject("Floor", planeGeometry, noSpecMaterial);
	gameObject->SetPosition(0.0f, 0.0f, 0.0f);
	gameObject->SetScale(1.0f, 1.0f, 1.0f);
	gameObject->SetRotation(0.0f, 0.0f, 0.0f);
	gameObject->SetTextureRV(_pGroundTextureRV);
	_pTerrainObject = gameObject;	

#pragma region CharacterCreation

	GameObject* normalParent;
	//---------------------------Body-----------------------------------------
	Geometry BodyGeometry = OBJLoader::Load(".\\Character\\Body.obj", false, _pd3dDevice); // 0
	gameObject = new GameObject("Body", BodyGeometry, shinyMaterial);
	gameObject->SetPosition(100.0f, 4.5f, 75.0f);
	gameObject->SetScale(1.0f, 1.0f, 1.0f);
	gameObject->SetRotation(0.0f, 0.0f, 0.0f);
	gameObject->SetTextureRV(_pTextureRV);
	gameObject->_IsLoadedObject = true;
	_bodyParent = gameObject;
	_gameObjects.push_back(gameObject);

	//---------------------------Head-----------------------------------------
	Geometry HeadGeometry = OBJLoader::Load(".\\Character\\Head.obj", false, _pd3dDevice); // 1
	gameObject = new GameObject("Head", HeadGeometry, shinyMaterial);
	gameObject->SetPosition(0.0f, 1.5f, 0.0f);
	gameObject->SetScale(1.0f, 1.0f, 1.0f);
	gameObject->SetRotation(0.0f, 0.0f, 0.0f);
	gameObject->SetTextureRV(_pTextureRV);
	gameObject->_IsLoadedObject = true;
	gameObject->SetParent(_bodyParent);
	_gameObjects.push_back(gameObject);

	//---------------------------LEFT ARM-----------------------------------------
	Geometry LeftTopArmGeometry = OBJLoader::Load(".\\Character\\Left_Top_Arm.obj", false, _pd3dDevice); // 2
	gameObject = new GameObject("Left_Top_Arm", LeftTopArmGeometry, shinyMaterial);
	gameObject->SetPosition(0.0f, 1.6f, -1.7f);
	gameObject->SetScale(1.0f, 1.0f, 1.0f);
	gameObject->SetRotation(90.0f, 0.0f, 0.0f);
	gameObject->SetTextureRV(_pTextureRV);
	gameObject->_IsLoadedObject = true;
	gameObject->SetParent(_bodyParent);
	normalParent = gameObject;
	_gameObjects.push_back(gameObject);

	Geometry LeftBottomArmGeometry = OBJLoader::Load(".\\Character\\Left_Bottom_Arm.obj", false, _pd3dDevice); // 3
	gameObject = new GameObject("Left_Bottom_Arm", LeftBottomArmGeometry, shinyMaterial);
	gameObject->SetPosition(0.0f, -1.5f, 0.0f);
	gameObject->SetScale(1.0f, 1.0f, 1.0f);
	gameObject->SetRotation(20.0f, 0.0f, 0.0f);
	gameObject->SetTextureRV(_pTextureRV);
	gameObject->_IsLoadedObject = true;
	gameObject->SetParent(normalParent);
	_gameObjects.push_back(gameObject);

	//---------------------------RIGHT ARM-----------------------------------------
	Geometry RightTopArmGeometry = OBJLoader::Load(".\\Character\\Right_Top_Arm.obj", false, _pd3dDevice); // 4
	gameObject = new GameObject("Right_Top_Arm", RightTopArmGeometry, shinyMaterial);
	gameObject->SetPosition(0.0f, 1.6f, 1.7f);
	gameObject->SetScale(1.0f, 1.0f, 1.0f);
	gameObject->SetRotation(0.0f, 0.0f, 0.0f);
	gameObject->SetTextureRV(_pTextureRV);
	gameObject->_IsLoadedObject = true;
	gameObject->SetParent(_bodyParent);
	normalParent = gameObject;
	_gameObjects.push_back(gameObject);

	Geometry RightBottomArmGeometry = OBJLoader::Load(".\\Character\\Right_Bottom_Arm.obj", false, _pd3dDevice); // 5
	gameObject = new GameObject("Right_Bottom_Arm", RightBottomArmGeometry, shinyMaterial);
	gameObject->SetPosition(0.0f, -1.5f, 0.0f);
	gameObject->SetScale(1.0f, 1.0f, 1.0f);
	gameObject->SetRotation(0.0f, 0.0f, 0.0f);
	gameObject->SetTextureRV(_pTextureRV);
	gameObject->_IsLoadedObject = true;
	gameObject->SetParent(normalParent);
	_gameObjects.push_back(gameObject);

	//---------------------------LEFT LEG-----------------------------------------
	Geometry LeftTopLegGeometry = OBJLoader::Load(".\\Character\\Left_Top_Leg.obj", false, _pd3dDevice); // 6
	gameObject = new GameObject("Left_Top_Leg", LeftTopLegGeometry, shinyMaterial);
	gameObject->SetPosition(0.0f, -1.3f, -0.5f);
	gameObject->SetScale(1.0f, 1.0f, 1.0f);
	gameObject->SetRotation(0.0f, 0.0f, 0.0f);
	gameObject->SetTextureRV(_pTextureRV);
	gameObject->_IsLoadedObject = true;
	gameObject->SetParent(_bodyParent);
	normalParent = gameObject;
	_gameObjects.push_back(gameObject);

	Geometry LeftBottomLegGeometry = OBJLoader::Load(".\\Character\\Left_Bottom_Leg.obj", false, _pd3dDevice); // 7
	gameObject = new GameObject("Left_Bottom_Leg", LeftBottomLegGeometry, shinyMaterial);
	gameObject->SetPosition(0.0f, -1.4f, 0.0f);
	gameObject->SetScale(1.0f, 1.0f, 1.0f);
	gameObject->SetRotation(0.0f, 0.0f, 0.0f);
	gameObject->SetTextureRV(_pTextureRV);
	gameObject->_IsLoadedObject = true;
	gameObject->SetParent(normalParent);
	_gameObjects.push_back(gameObject);

	//---------------------------RIGHT LEG-----------------------------------------
	Geometry RightTopLegGeometry = OBJLoader::Load(".\\Character\\Right_Top_Leg.obj", false, _pd3dDevice); // 8
	gameObject = new GameObject("Right_Top_Leg", RightTopLegGeometry, shinyMaterial);
	gameObject->SetPosition(0.0f, -1.3f, 0.5f);
	gameObject->SetScale(1.0f, 1.0f, 1.0f);
	gameObject->SetRotation(0.0f, 0.0f, 0.0f);
	gameObject->SetTextureRV(_pTextureRV);
	gameObject->_IsLoadedObject = true;
	gameObject->SetParent(_bodyParent);
	normalParent = gameObject;
	_gameObjects.push_back(gameObject);

	Geometry RightBottomLegGeometry = OBJLoader::Load(".\\Character\\Right_Bottom_Leg.obj", false, _pd3dDevice); // 9
	gameObject = new GameObject("Right_Bottom_Leg", RightBottomLegGeometry, shinyMaterial);
	gameObject->SetPosition(0.0f, -1.4f, 0.0f);
	gameObject->SetScale(1.0f, 1.0f, 1.0f);
	gameObject->SetRotation(0.0f, 0.0f, 0.0f);
	gameObject->SetTextureRV(_pTextureRV);
	gameObject->_IsLoadedObject = true;
	gameObject->SetParent(normalParent);
	_gameObjects.push_back(gameObject);

#pragma endregion

#pragma region CharacterAnimation

	std::ifstream inFile;
	inFile.open(".\\Character\\Animation.txt", ios::in);

	if (!inFile.good())
	{
		
	}
	else
	{
		std::string input;
		int bone = 0;

		while (!inFile.eof())
		{
			inFile >> input; 
			Frame newframe;
	

			if (input.compare("b") == 0) 
			{
				inFile >> bone;
				CharacterAnimation.push_back(vector<Frame>());	
			}
			if (input.compare("pos") == 0)
			{				
				inFile >> newframe.Postion.x;
				inFile >> newframe.Postion.y;
				inFile >> newframe.Postion.z;
			}
			if (input.compare("rot") == 0)
			{			
				inFile >> newframe.Rotation.x;
				inFile >> newframe.Rotation.y;
				inFile >> newframe.Rotation.z;
			}
			if (input.compare("sca") == 0) 
			{				
				inFile >> newframe.Scale.x;
				inFile >> newframe.Scale.y;
				inFile >> newframe.Scale.z;
			}
			if (input.compare("fra") == 0)
			{
				inFile >> newframe.frametime;
				newframe.bone = bone;
				CharacterAnimation.back().push_back(newframe);
			}
		}
	}

#pragma endregion

	return S_OK;
}

HRESULT Application::InitShadersAndInputLayout()
{
	HRESULT hr;

    // Compile the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    hr = CompileShaderFromFile(L"DX11 Framework.fx", "VS", "vs_4_0", &pVSBlob);

    if (FAILED(hr))
    {
        MessageBox(nullptr,
                   L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

	// Create the vertex shader
	hr = _pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &_pVertexShader);

	if (FAILED(hr))
	{	
		pVSBlob->Release();
        return hr;
	}

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
    hr = CompileShaderFromFile(L"DX11 Framework.fx", "PS", "ps_4_0", &pPSBlob);

    if (FAILED(hr))
    {
        MessageBox(nullptr,
                   L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

	_pTerrainClass = new TerrainClass();
	if (!_pTerrainClass)
		return false;

	hr = _pTerrainClass->Initialize(_pd3dDevice, ".\\Resources\\HeightMap.bmp", L"Resources\\stone.dds");
	if (FAILED(hr))
		return hr;

	_pHeightTerrainClass = new TerrainClass();
	if (!_pTerrainClass)
		return false;
	_pHeightTerrainClass->SetGenerationType(TerrainClass::HEIGHTMAP);

	hr = _pHeightTerrainClass->Initialize(_pd3dDevice, ".\\Resources\\HeightMap.bmp", L"Resources\\stone.dds");
	if (FAILED(hr))
		return hr;

	_pVoxelTerrainClass = new VoxelTerrain(); // Re-add



	_pTerrainColour = new TerrainColour();
	if (!_pTerrainColour)
		return false;

	hr = _pTerrainColour->Initialize(_pd3dDevice, nullptr);
	if (FAILED(hr))
		return hr;

	// Create the pixel shader
	hr = _pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &_pPixelShader);
	pPSBlob->Release();

    if (FAILED(hr))
        return hr;
	
    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = ARRAYSIZE(layout);

    // Create the input layout
	hr = _pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
                                        pVSBlob->GetBufferSize(), &_pVertexLayout);
	pVSBlob->Release();

	if (FAILED(hr))
        return hr;

    // Set the input layout
    _pImmediateContext->IASetInputLayout(_pVertexLayout);

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = _pd3dDevice->CreateSamplerState(&sampDesc, &_pSamplerLinear);

	return hr;
}

HRESULT Application::InitVertexBuffer()
{
	HRESULT hr;

    // Create vertex buffer
    SimpleVertex vertices[] =
    {
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },

		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
    };

    D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 24;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;

    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pVertexBuffer);

    if (FAILED(hr))
        return hr;

	// Create vertex buffer
	SimpleVertex planeVertices[] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 5.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(5.0f, 5.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(5.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
	};

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = planeVertices;

	hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pPlaneVertexBuffer);

	if (FAILED(hr))
		return hr;

	return S_OK;
}

HRESULT Application::InitIndexBuffer()
{
	HRESULT hr;

    // Create index buffer
    WORD indices[] =
    {
		3, 1, 0,
		2, 1, 3,

		6, 4, 5,
		7, 4, 6,

		11, 9, 8,
		10, 9, 11,

		14, 12, 13,
		15, 12, 14,

		19, 17, 16,
		18, 17, 19,

		22, 20, 21,
		23, 20, 22
    };

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * 36;     
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = indices;
    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pIndexBuffer);

    if (FAILED(hr))
        return hr;

	// Create plane index buffer
	WORD planeIndices[] =
	{
		0, 3, 1,
		3, 2, 1,
	};

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD) * 6;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = planeIndices;
	hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pPlaneIndexBuffer);

	if (FAILED(hr))
		return hr;

	return S_OK;
}

HRESULT Application::InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW );
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // Create window
    _hInst = hInstance;
    RECT rc = {0, 0, 960, 540};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    _hWnd = CreateWindow(L"TutorialWindowClass", L"FGGC Semester 2 Framework", WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                         nullptr);
    if (!_hWnd)
		return E_FAIL;

    ShowWindow(_hWnd, nCmdShow);
	UpdateWindow(_hWnd);

    return S_OK;
}

HRESULT Application::CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, ppBlobOut, &pErrorBlob);

    if (FAILED(hr))
    {
        if (pErrorBlob != nullptr)
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());

        if (pErrorBlob) pErrorBlob->Release();

        return hr;
    }

    if (pErrorBlob) pErrorBlob->Release();

    return S_OK;
}

HRESULT Application::InitDevice()
{
    HRESULT hr = S_OK;

    UINT createDeviceFlags = 0;

#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };

    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	UINT sampleCount = 4;

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = _renderWidth;
    sd.BufferDesc.Height = _renderHeight;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = _hWnd;
	sd.SampleDesc.Count = sampleCount;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        _driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain(nullptr, _driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                                           D3D11_SDK_VERSION, &sd, &_pSwapChain, &_pd3dDevice, &_featureLevel, &_pImmediateContext);
        if (SUCCEEDED(hr))
            break;
    }

    if (FAILED(hr))
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = _pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    if (FAILED(hr))
        return hr;

    hr = _pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &_pRenderTargetView);
    pBackBuffer->Release();

    if (FAILED(hr))
        return hr;

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)_renderWidth;
    vp.Height = (FLOAT)_renderHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    _pImmediateContext->RSSetViewports(1, &vp);

	InitShadersAndInputLayout();

	InitVertexBuffer();
	InitIndexBuffer();

    // Set primitive topology
    _pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create the constant buffer
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
    hr = _pd3dDevice->CreateBuffer(&bd, nullptr, &_pConstantBuffer);

    if (FAILED(hr))
        return hr;

	D3D11_TEXTURE2D_DESC depthStencilDesc;

	depthStencilDesc.Width = _renderWidth;
	depthStencilDesc.Height = _renderHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = sampleCount;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	_pd3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &_depthStencilBuffer);
	_pd3dDevice->CreateDepthStencilView(_depthStencilBuffer, nullptr, &_depthStencilView);

	_pImmediateContext->OMSetRenderTargets(1, &_pRenderTargetView, _depthStencilView);

	// Rasterizer
	D3D11_RASTERIZER_DESC cmdesc;

	ZeroMemory(&cmdesc, sizeof(D3D11_RASTERIZER_DESC));
	cmdesc.FillMode = D3D11_FILL_SOLID;
	cmdesc.CullMode = D3D11_CULL_NONE;
	hr = _pd3dDevice->CreateRasterizerState(&cmdesc, &RSCullNone);

	D3D11_DEPTH_STENCIL_DESC dssDesc;
	ZeroMemory(&dssDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	dssDesc.DepthEnable = true;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dssDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	_pd3dDevice->CreateDepthStencilState(&dssDesc, &DSLessEqual);

	ZeroMemory(&cmdesc, sizeof(D3D11_RASTERIZER_DESC));

	cmdesc.FillMode = D3D11_FILL_SOLID;
	cmdesc.CullMode = D3D11_CULL_BACK;

	cmdesc.FrontCounterClockwise = true;
	hr = _pd3dDevice->CreateRasterizerState(&cmdesc, &CCWcullMode);

	cmdesc.FrontCounterClockwise = false;
	hr = _pd3dDevice->CreateRasterizerState(&cmdesc, &CWcullMode);

    return S_OK;
}

void Application::Cleanup()
{
    if (_pImmediateContext) _pImmediateContext->ClearState();
	if (_pSamplerLinear) _pSamplerLinear->Release();

	if (_pTextureRV) _pTextureRV->Release();

	if (_pGroundTextureRV) _pGroundTextureRV->Release();

    if (_pConstantBuffer) _pConstantBuffer->Release();

    if (_pVertexBuffer) _pVertexBuffer->Release();
    if (_pIndexBuffer) _pIndexBuffer->Release();
	if (_pPlaneVertexBuffer) _pPlaneVertexBuffer->Release();
	if (_pPlaneIndexBuffer) _pPlaneIndexBuffer->Release();

    if (_pVertexLayout) _pVertexLayout->Release();
    if (_pVertexShader) _pVertexShader->Release();
    if (_pPixelShader) _pPixelShader->Release();
    if (_pRenderTargetView) _pRenderTargetView->Release();
    if (_pSwapChain) _pSwapChain->Release();
    if (_pImmediateContext) _pImmediateContext->Release();
    if (_pd3dDevice) _pd3dDevice->Release();
	if (_depthStencilView) _depthStencilView->Release();
	if (_depthStencilBuffer) _depthStencilBuffer->Release();

	if (DSLessEqual) DSLessEqual->Release();
	if (RSCullNone) RSCullNone->Release();

	if (CCWcullMode) CCWcullMode->Release();
	if (CWcullMode) CWcullMode->Release();

	if (_camera)
	{
		delete _camera;
		_camera = nullptr;
	}

	for (auto gameObject : _gameObjects)
	{
		if (gameObject)
		{
			delete gameObject;
			gameObject = nullptr;
		}
	}
}

void Application::moveForward(int objectNumber)
{
	XMFLOAT3 position = _gameObjects[objectNumber]->GetPosition();
	position.z -= 0.1f;
	_gameObjects[objectNumber]->SetPosition(position);
}

void Application::moveBackwards(int objectNumber)
{
	XMFLOAT3 position = _gameObjects[objectNumber]->GetPosition();
	position.z += 0.1f;
	_gameObjects[objectNumber]->SetPosition(position);
}

void Application::moveLeft(int objectNumber)
{
	XMFLOAT3 position = _gameObjects[objectNumber]->GetPosition();
	position.x -= 0.1f;
	_gameObjects[objectNumber]->SetPosition(position);
}
void Application::moveRight(int objectNumber)
{
	XMFLOAT3 position = _gameObjects[objectNumber]->GetPosition();
	position.x += 0.1f;
	_gameObjects[objectNumber]->SetPosition(position);
}


void Application::HandleInput(float delta)
{
	if (GetAsyncKeyState('1')) // Default
	{
		//moveForward(1);
		_pTerrainClass->ResetTerrain(_pImmediateContext);
	}
	float characterrot = 0.0f;
	bool upbutton = false;
	bool downbutton = false;
	bool pushed = false;

	if (GetAsyncKeyState(VK_UP))
	{
		upbutton = true;
		pushed = true;
		characterrot += 90.0f;
		moveBackwards(0);
		AnimateCharacter(delta);
	}
	if (GetAsyncKeyState(VK_DOWN))
	{
		pushed = true;
		downbutton = true;
		characterrot += -90.0f;

		moveForward(0);
		AnimateCharacter(delta);
	}
	if (GetAsyncKeyState(VK_LEFT))
	{	
		pushed = true;
		if (upbutton)
			characterrot += -45.0f;
		if (downbutton)
			characterrot += 45.0f;

		moveLeft(0);
		AnimateCharacter(delta);
	}
	if (GetAsyncKeyState(VK_RIGHT))
	{
		pushed = true;
		if (upbutton)
			characterrot += 45.0f;
		else if(downbutton)
			characterrot += -45.0f;
		else
			characterrot += -180.0f;

		moveRight(0);
		AnimateCharacter(delta);
	}
	if (pushed)
	{
		UpdateCharacterHeight();
		_bodyParent->SetRotation(_bodyParent->GetRotation().x, XMConvertToRadians(characterrot), _bodyParent->GetRotation().z);
	}
		

	//------------------------------Move Camera-----------------------------
	if (GetAsyncKeyState('W'))
	{
		_camera->Walk(_cameraSpeed * delta);
	}
	if (GetAsyncKeyState('S'))
	{
		_camera->Walk(-_cameraSpeed * delta);
	}
	if (GetAsyncKeyState('D'))
	{
		_camera->Strafe(_cameraSpeed * delta);
	}
	if (GetAsyncKeyState('A'))
	{
		_camera->Strafe(-_cameraSpeed * delta);
	}

	if ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) && (_RightMouseDown == false))
	{
		POINT cursor;
		GetCursorPos(&cursor);
		_CursorX = cursor.x;
		_CursorY = cursor.y;
		_RightMouseDown = true;//reset the flag
	}
	else if (GetAsyncKeyState(VK_RBUTTON) == 0)
	{
		_RightMouseDown = false;//reset the flag
	}
	else if ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) && (_RightMouseDown == true))
	{
		POINT cursor;
		GetCursorPos(&cursor);

		_camera->Pitch((cursor.y - _CursorY) * delta * _cameraPivotSpeed);
		_camera->RotateY((cursor.x - _CursorX) * delta * _cameraPivotSpeed);

		_RightMouseDown = false;
	}

}

void Application::UpdateCharacterHeight()
{
	bool foundHeight = false;

	float posX = _bodyParent->GetPosition().x;
	float posZ = _bodyParent->GetPosition().z;

	foundHeight = _pTerrainClass->GetHeightAtPosition(posX, posZ, _characterHeight);
	if (foundHeight)
	{
		_bodyParent->SetPosition(posX, _characterHeight + _characterOffset, posZ);
	}
}

float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}

void Application::AnimateCharacter(float delta)
{
	float maxseconds = 1.0f;

	if (isReversingFrame)
	{
		_frame -= delta;
		if (_frame <= 0.1f)
		{
			_frame = maxseconds;
			_currentFrame--;

			if (_currentFrame < 0)
			{
				isReversingFrame = false;
				_currentFrame = 0;
			}			
		}			
	}
	else
	{
		_frame += delta;

		if (_frame >= maxseconds)
		{
			_frame = 0.1f;
			_currentFrame++;

			if (_currentFrame > 9)
			{
				isReversingFrame = true;
				_currentFrame = 9;
			}				
		}			
	}

	float frameaverage = 1 / maxseconds;

	for (int i = 0; i < CharacterAnimation.size(); i++)
	{
		if (_currentFrame == 9)
			break;

		float rot1 = lerp(CharacterAnimation[i][_currentFrame].Rotation.z, CharacterAnimation[i][_currentFrame + 1].Rotation.z, (_frame * frameaverage));

		int currentbone = CharacterAnimation[i][_currentFrame].bone;

		_gameObjects[currentbone]->SetRotation(_gameObjects[currentbone]->GetRotation().x, _gameObjects[currentbone]->GetRotation().y, XMConvertToRadians(rot1));
	}
}
void Application::HandleImGui(double delta)
{
	bool resetTerrain = false;
	bool applyDiamondSquare = false;
	bool applyFaultLine = false;
	bool applyHillCircle = false;
	bool applyParticle = false;
	bool applySmooth = false;


	ImGui_ImplDX11_NewFrame();

	if (ImGui::SliderFloat("Character Offset", &_characterOffset, 0.0f, 10.0f))
	{
		UpdateCharacterHeight();
	}

	ImGui::SliderInt("Terrain Type ", &_iTerrainState, 0, 2);
	ImGui::SliderFloat("Camera Speed", &_cameraSpeed, 0.0f, 50.0f);

	if (ImGui::Button("Reset Terrain")) resetTerrain ^= 1;

	if (ImGui::Button("DiamondSquare")) applyDiamondSquare ^= 1;
	ImGui::InputFloat("Height", &_DSHeight, 1.0f, 50.0f);

	if (ImGui::Button("FaultLine")) applyFaultLine ^= 1;
	ImGui::InputInt("Amount of Lines", &_FLNumberOfLines, 1, 300);
	ImGui::InputFloat("Line Height", &_FLHeight, 1.0f, 50.0f);

	if (ImGui::Button("Hill Circle")) applyHillCircle ^= 1; // add iterations

	if (ImGui::Button("Particle Deposition")) applyParticle ^= 1;
	
	if (ImGui::Checkbox("Wireframe", &_isTerrainWireframe))
	{
		_pTerrainClass->SetWireFrame(_isTerrainWireframe);
		_pHeightTerrainClass->SetWireFrame(_isTerrainWireframe);
	}

	if (ImGui::Button("Smooth")) applySmooth ^= 1;

	bool open = true;
	if (_iTerrainState == 2)
	{
		ImGui::Begin("Voxel Controls", &open, ImVec2(200, 300), ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::SetWindowPos(ImVec2(74, 516));
		ImGui::SetWindowSize(ImVec2(400, 500));

		ImGui::InputInt("Offset Height", _pVoxelTerrainClass->GetHeightParam());
		ImGui::InputInt("Offset Width", _pVoxelTerrainClass->GetWidthParam());
		ImGui::InputInt("Offset Depth", _pVoxelTerrainClass->GetDepthParam());

		ImGui::InputInt("Chunk Height", _pVoxelTerrainClass->GetChunkHeightParam());
		ImGui::InputInt("Chunk Width", _pVoxelTerrainClass->GetChunkWidthParam());
		ImGui::InputInt("Chunk Depth", _pVoxelTerrainClass->GetChunkDepthParam());		

		ImGui::SliderFloat("Pos X", _pVoxelTerrainClass->GetTerrainPosX(), -4.0f, 20.0f);
		ImGui::SliderFloat("Pos Y", _pVoxelTerrainClass->GetTerrainPosY(), -4.0f, 20.0f);
		ImGui::SliderFloat("Pos Z", _pVoxelTerrainClass->GetTerrainPosZ(), -4.0f, 20.0f);

		ImGui::SliderFloat("CutOff Height", _pVoxelTerrainClass->GetCuttoffHeight(), 0.01f, 0.01f);
		ImGui::Checkbox("Invert Height", _pVoxelTerrainClass->GetCuttoffInverted());

		ImGui::End();
	}

	//bool open = false;
	//ImGui::Begin("Test Window", &open, ImVec2(200, 300), ImGuiWindowFlags_AlwaysAutoResize);
	////ImGui::SetWindowPos(ImVec2(74, 516));
	//string text = "Window Pos X:" + std::to_string(ImGui::GetWindowPos().x) + " Y:" + std::to_string(ImGui::GetWindowPos().y);

	//ImGui::Text(text.c_str());
	////ImGui::Begin(ImVec2(50.0f, 50.0f), ImVec2(200, 300), "Test Options", "Gaussian Blur", true, ImGuiWindowFlags_AlwaysAutoResize);

	//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	//ImGui::End();

	if (resetTerrain)
	{
		_pTerrainClass->ResetTerrain(_pImmediateContext);
	}
	if (applyDiamondSquare)
	{
		_pTerrainClass->ApplyDiamondSquare(_pImmediateContext, _DSHeight);
	}
	if (applyFaultLine)
	{
		_pTerrainClass->ApplyFaultLine(_pImmediateContext, _FLNumberOfLines, _FLHeight);
	}
	if (applyHillCircle)
	{
		_pTerrainClass->ApplyHillCircle(_pImmediateContext, 1);
	}
	if (applyParticle)
	{
		_pTerrainClass->ApplyParticleDeposition(_pImmediateContext, 300);		
	}
	if (applySmooth)
	{
		_pTerrainClass->ApplySmoothing(_pImmediateContext, 0.1f);
	}
}

void Application::Update(double deltatime)
{
    // Update our time
    static float timeSinceStart = 0.0f;
    static DWORD dwTimeStart = 0;

    DWORD dwTimeCur = GetTickCount();

    if (dwTimeStart == 0)
        dwTimeStart = dwTimeCur;

	timeSinceStart = (dwTimeCur - dwTimeStart) / 1000.0f;
	HandleInput(deltatime);

	// Update camera
	_camera->UpdateViewMatrix();

	HandleImGui(deltatime);

	for (auto gameObject : _gameObjects)
	{
		gameObject->Update(timeSinceStart);
	}

	_pVoxelTerrainClass->Update(timeSinceStart);

	_pTerrainObject->Update(timeSinceStart);
}

void Application::Draw()
{
	float ClearColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
    _pImmediateContext->ClearRenderTargetView(_pRenderTargetView, ClearColor);
	_pImmediateContext->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    ConstantBuffer cb;

	cb.View = DirectX::XMMatrixTranspose(_camera->View());
	cb.Projection = DirectX::XMMatrixTranspose(_camera->Proj());	
	cb.light = basicLight;
	cb.EyePosW = _camera->GetPosition();

	_pImmediateContext->IASetInputLayout(_pVertexLayout);

	_pImmediateContext->VSSetShader(_pVertexShader, nullptr, 0);
	_pImmediateContext->PSSetShader(_pPixelShader, nullptr, 0);

	_pImmediateContext->VSSetConstantBuffers(0, 1, &_pConstantBuffer);
	_pImmediateContext->PSSetConstantBuffers(0, 1, &_pConstantBuffer);
	_pImmediateContext->PSSetSamplers(0, 1, &_pSamplerLinear);

	for (int i = 0; i < _gameObjects.size(); i++)
	{
		// Get render material
		Material material = _gameObjects[i]->GetMaterial();

		// Copy material to shader
		cb.surface.AmbientMtrl = material.ambient;
		cb.surface.DiffuseMtrl = material.diffuse;
		cb.surface.SpecularMtrl = material.specular;	

		// Set world matrix
		cb.World = DirectX::XMMatrixTranspose(_gameObjects[i]->GetWorldMatrix());

		if (_gameObjects[i]->HasTexture())
		{
			ID3D11ShaderResourceView * textureRV = _gameObjects[i]->GetTextureRV();
			_pImmediateContext->PSSetShaderResources(0, 1, &textureRV);
			cb.HasTexture = 1.0f;
		}
		else
		{
			cb.HasTexture = 0.0f;
		}
		
		_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
	
		if(_iTerrainState != 2)
			_gameObjects[i]->Draw(_pImmediateContext);

	}

	cb.World = DirectX::XMMatrixTranspose(_pTerrainObject->GetWorldMatrix());
	
	_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	switch (_iTerrainState)
	{
	case 0: // Height
			_pHeightTerrainClass->Render(_pImmediateContext);
			_pTerrainColour->Render(_pImmediateContext, _pHeightTerrainClass->GetIndexCount(), cb.World, cb.View, cb.Projection, basicLight.AmbientLight, basicLight.DiffuseLight, basicLight.LightVecW, _pHeightTerrainClass->GetTexture());
		break;

	case 1:
		    _pTerrainClass->Render(_pImmediateContext);
			_pTerrainColour->Render(_pImmediateContext, _pTerrainClass->GetIndexCount(), cb.World, cb.View, cb.Projection, basicLight.AmbientLight, basicLight.DiffuseLight, basicLight.LightVecW, _pTerrainClass->GetTexture());
		break;
	case 2:
			_pVoxelTerrainClass->Draw(_pImmediateContext, cb, _pConstantBuffer); 
		break;
	default:
		break;
	}
	
	
	
	
	ImGui::Render();
    _pSwapChain->Present(0, 0);
}