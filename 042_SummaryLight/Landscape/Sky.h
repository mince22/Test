#pragma once
#include "../Executes/Execute.h"

class Sky
{
public:
	Sky(ExecuteValues* values);
	~Sky();

	void Update();
	void PreRender();
	void Render();
	void PostRender();
	void ResizeScreen() {}

private:
	void GenerateSphere();
	void GenerateQuad();

	void GetStarIntensity();

private:
	struct ExecuteValues* values;
	bool realTime;

	float theta;
	float phi;
	float prevTheta, prevPhi;

	RenderTarget* mieTarget, *rayleighTarget;
	Shader* scatterShader;
	Shader* targetShader;
	Shader* moonShader;

	UINT vertexCount, indexCount;
	UINT radius, slices, stacks;

	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;

	VertexTexture* quadVertices;
	ID3D11Buffer* quadBuffer;

	Render2D* rayleigh2D;
	Render2D* mie2D;

	WorldBuffer* worldBuffer;
	WorldBuffer* moonWorldBuffer;

	DepthStencilState* depthState[2];
	RasterizerState* moonRasterizer[2];

	Texture* starField;
	Texture* moon;
	float distanceMoon;
	BlendState* blendState[2];

	SamplerState* rayleighSampler;
	SamplerState* mieSampler;
	SamplerState* starSampler;

private:
	class ScatterBuffer : public ShaderBuffer
	{
	public:
		ScatterBuffer() : ShaderBuffer(&Data, sizeof(Data))
		{
			Data.StarIntensity = 0.0f;
		}

		struct Struct
		{
			float StarIntensity;

			float Padding[3];
		} Data;
	};

	ScatterBuffer* scatterBuffer;

private:
	class TargetBuffer : public ShaderBuffer
	{
	public:
		TargetBuffer() : ShaderBuffer(&Data, sizeof(Data))
		{
			Data.WaveLength = D3DXVECTOR3(0.65f, 0.57f, 0.465f);

			Data.InvWaveLength.x = 1.0f / powf(Data.WaveLength.x, 4.0f);
			Data.InvWaveLength.y = 1.0f / powf(Data.WaveLength.y, 4.0f);
			Data.InvWaveLength.z = 1.0f / powf(Data.WaveLength.z, 4.0f);

			Data.WaveLengthMie.x = powf(Data.WaveLength.x, -0.84f);
			Data.WaveLengthMie.y = powf(Data.WaveLength.y, -0.84f);
			Data.WaveLengthMie.z = powf(Data.WaveLength.z, -0.84f);

			Data.SampleCount = 20;
		}

		struct Struct
		{
			D3DXVECTOR3 WaveLength;
			int SampleCount;

			D3DXVECTOR3 InvWaveLength;
			float Padding;

			D3DXVECTOR3 WaveLengthMie;
			float Padding2;
		} Data;
	};

	TargetBuffer* targetBuffer;
};