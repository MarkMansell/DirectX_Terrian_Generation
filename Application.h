#pragma once

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include "DDSTextureLoader.h"
#include "resource.h"
#include "Camera.h"
#include <iostream>

#include "VoxelTerrain.h"
#include "GameObject.h"
#include "OBJLoader.h"
#include "Structures.h"
#include "TerrainClass.h"
#include "TerrainColour.h"
#include "Structures.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>
#include <vector>
using namespace DirectX;

class Application
{
private:
	HINSTANCE               _hInst;
	HWND                    _hWnd;
	D3D_DRIVER_TYPE         _driverType;
	D3D_FEATURE_LEVEL       _featureLevel;
	ID3D11VertexShader*     _pVertexShader;
	ID3D11PixelShader*      _pPixelShader;
	ID3D11InputLayout*      _pVertexLayout;

	ID3D11Buffer*           _pVertexBuffer;
	ID3D11Buffer*           _pIndexBuffer;

	ID3D11Buffer*           _pPlaneVertexBuffer;
	ID3D11Buffer*           _pPlaneIndexBuffer;

	ID3D11Buffer*           _pConstantBuffer;

	ID3D11DepthStencilView* _depthStencilView = nullptr;
	ID3D11Texture2D* _depthStencilBuffer = nullptr;

	ID3D11ShaderResourceView * _pTextureRV = nullptr;

	ID3D11ShaderResourceView * _pGroundTextureRV = nullptr;

	ID3D11SamplerState * _pSamplerLinear = nullptr;

	TerrainClass*			_pTerrainClass;
	TerrainClass*			_pHeightTerrainClass;
	VoxelTerrain*           _pVoxelTerrainClass;
	TerrainColour*          _pTerrainColour;
	GameObject*             _pTerrainObject;

	int                     _iTerrainState;

	bool					_isTerrainWireframe;

	Light basicLight;

	vector< vector<Frame> > CharacterAnimation;

	vector<GameObject *> _gameObjects;

	GameObject* _bodyParent;
	float _characterHeight = 0.0f;
	float _characterOffset = 4.5f;
	float _frame;
	int _currentFrame = 0;
	bool isReversingFrame;

	Camera * _camera;
	float _cameraSpeed = 40.0f;
	float _cameraPivotSpeed = 3.0f;

	float _DSHeight = 10.0f;

	int _FLNumberOfLines = 1;
	float _FLHeight = 1.0f;

	bool _RightMouseDown = false;

	float _CursorX;
	float _CursorY;

	UINT _WindowHeight;
	UINT _WindowWidth;

	UINT _renderHeight = 1020; // 1080
	UINT _renderWidth = 1920;

	ID3D11DepthStencilState* DSLessEqual;
	ID3D11RasterizerState* RSCullNone;

	ID3D11RasterizerState* CCWcullMode;
	ID3D11RasterizerState* CWcullMode;

private:
	HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
	HRESULT InitDevice();
	void Cleanup();
	HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
	HRESULT InitShadersAndInputLayout();
	HRESULT InitVertexBuffer();
	HRESULT InitIndexBuffer();

	void moveForward(int objectNumber);
	void moveBackwards(int objectNumber);
	void moveLeft(int objectNumber);
	void moveRight(int objectNumber);

	void HandleInput(float delta);
	void HandleImGui(double delta);
	void AnimateCharacter(float delta);
	void UpdateCharacterHeight();

public:
	Application();
	~Application();

	HRESULT Initialise(HINSTANCE hInstance, int nCmdShow);

	bool HandleKeyboard(MSG msg);

	void Update(double delta);
	void Draw();
};

