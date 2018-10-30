#include "stdafx.h"
#include "Sky.h"
#include "../Lights/Projection.h"

Sky::Sky(ExecuteValues * values)
	: values(values)
	, realTime(false), theta(0), phi(0), distanceMoon(100.0f)
	, radius(10), slices(32), stacks(16)
{
	mieTarget = new RenderTarget(values, 128, 64);
	rayleighTarget = new RenderTarget(values, 128, 64);

	scatterShader = new Shader(Shaders + L"044_Scattering.hlsl", "VS_Scattering", "PS_Scattering");
	targetShader = new Shader(Shaders + L"044_Scattering.hlsl", "VS_Target", "PS_Target");
	moonShader = new Shader(Shaders + L"044_Scattering.hlsl", "VS_Moon", "PS_Moon");

	worldBuffer = new WorldBuffer();
	moonWorldBuffer = new WorldBuffer();
	targetBuffer = new TargetBuffer();
	scatterBuffer = new ScatterBuffer();

	GenerateSphere();
	GenerateQuad();

	depthState[0] = new DepthStencilState();
	depthState[1] = new DepthStencilState();
	depthState[1]->DepthEnable(false);
	depthState[1]->DepthWriteEnable(D3D11_DEPTH_WRITE_MASK_ZERO);

	rayleigh2D = new Render2D(values);
	rayleigh2D->Position(0, 100);
	rayleigh2D->Scale(200, 100);

	mie2D = new Render2D(values);
	mie2D->Position(0, 0);
	mie2D->Scale(200, 100);

	starField = new Texture(Textures + L"starfield.png");
	moon = new Texture(Textures + L"moon.tga");

	rayleighSampler = new SamplerState();
	rayleighSampler->AddressU(D3D11_TEXTURE_ADDRESS_CLAMP);
	rayleighSampler->AddressV(D3D11_TEXTURE_ADDRESS_CLAMP);
	rayleighSampler->AddressW(D3D11_TEXTURE_ADDRESS_CLAMP);
	rayleighSampler->Filter(D3D11_FILTER_MIN_MAG_MIP_LINEAR);

	mieSampler = new SamplerState();
	mieSampler->AddressU(D3D11_TEXTURE_ADDRESS_CLAMP);
	mieSampler->AddressV(D3D11_TEXTURE_ADDRESS_CLAMP);
	mieSampler->AddressW(D3D11_TEXTURE_ADDRESS_CLAMP);
	mieSampler->Filter(D3D11_FILTER_MIN_MAG_MIP_LINEAR);

	starSampler = new SamplerState();
	starSampler->AddressU(D3D11_TEXTURE_ADDRESS_CLAMP);
	starSampler->AddressV(D3D11_TEXTURE_ADDRESS_CLAMP);
	starSampler->AddressW(D3D11_TEXTURE_ADDRESS_CLAMP);
	starSampler->Filter(D3D11_FILTER_MIN_MAG_MIP_LINEAR);

	moonRasterizer[0] = new RasterizerState();
	moonRasterizer[1] = new RasterizerState();
	moonRasterizer[1]->CullMode(D3D11_CULL_NONE);

	blendState[0] = new BlendState();
	blendState[1] = new BlendState();
	blendState[1]->BlendEnable(true);
}

Sky::~Sky()
{
	SAFE_DELETE(rayleighSampler);
	SAFE_DELETE(mieSampler);
	SAFE_DELETE(starSampler);

	SAFE_DELETE(depthState[0]);
	SAFE_DELETE(depthState[1]);

	SAFE_DELETE_ARRAY(quadVertices);
	SAFE_RELEASE(quadBuffer);

	SAFE_DELETE(worldBuffer);
	SAFE_DELETE(targetBuffer);
	SAFE_DELETE(scatterBuffer);

	SAFE_RELEASE(vertexBuffer);
	SAFE_RELEASE(indexBuffer);

	SAFE_DELETE(scatterShader);
	SAFE_DELETE(targetShader);

	SAFE_DELETE(mieTarget);
	SAFE_DELETE(rayleighTarget);
}

void Sky::Update()
{
	D3DXMATRIX V;
	D3DXVECTOR3 pos;
	values->MainCamera->Position(&pos);
	D3DXMatrixTranslation(&V, pos.x, pos.y, pos.z);

	worldBuffer->SetMatrix(V);
}

void Sky::PreRender()
{
	if (prevTheta == theta && prevPhi == phi) return;

	mieTarget->Set();
	rayleighTarget->Set();

	ID3D11RenderTargetView * rtvs[2];
	rtvs[0] = rayleighTarget->GetRTV();
	rtvs[1] = mieTarget->GetRTV();

	//   멀티 안됨, 통일 시켜야함
	ID3D11DepthStencilView * dsv;
	dsv = rayleighTarget->GetDSV();

	D3D::Get()->SetRenderTargets(2, rtvs, dsv);

	UINT stride = sizeof(VertexTexture);
	UINT offset = 0;

	D3D::GetDC()->IASetVertexBuffers(0, 1, &quadBuffer, &stride, &offset);
	D3D::GetDC()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	targetBuffer->SetPSBuffer(11);
	targetShader->Render();

	D3D::GetDC()->Draw(6, 0);
}

void Sky::Render()
{
	UINT stride = sizeof(VertexTexture);
	UINT offset = 0;

	D3D::GetDC()->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	D3D::GetDC()->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	D3D::GetDC()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	scatterShader->Render();
	worldBuffer->SetVSBuffer(1);

	ID3D11ShaderResourceView * rSRV = rayleighTarget->GetSRV();
	D3D::GetDC()->PSSetShaderResources(10, 1, &rSRV);

	ID3D11ShaderResourceView * mSRV = mieTarget->GetSRV();
	D3D::GetDC()->PSSetShaderResources(11, 1, &mSRV);

	rayleighSampler->PSSetSamplers(10);
	mieSampler->PSSetSamplers(11);
	starSampler->PSSetSamplers(12);

	starField->SetShaderResource(12);
	scatterBuffer->Data.StarIntensity = values->GlobalLight->Data.Direction.y;
	scatterBuffer->SetPSBuffer(10);

	depthState[1]->OMSetDepthStencilState();
	D3D::GetDC()->DrawIndexed(indexCount, 0, 0);
	depthState[0]->OMSetDepthStencilState();

	rayleigh2D->SRV(rayleighTarget->GetSRV());
	rayleigh2D->Update();
	rayleigh2D->Render();

	mie2D->SRV(mieTarget->GetSRV());
	mie2D->Update();
	mie2D->Render();

	D3D::GetDC()->IASetVertexBuffers(0, 1, &quadBuffer, &stride, &offset);
	D3D::GetDC()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ImGui::SliderFloat("Theta", &theta, 0.0f, Math::PI);
	ImGui::SliderFloat("Phi", &phi, 0.0f, Math::PI * 2.0f);
	values->GlobalLight->Data.Direction.x = sinf(theta) * cosf(phi);
	values->GlobalLight->Data.Direction.y = sinf(theta) * sinf(phi);
	values->GlobalLight->Data.Direction.z = cosf(theta);

	D3DXMATRIX s;
	D3DXMatrixScaling(&s, 10.0f, 10.0f, 10.0f);

	D3DXMATRIX t;
	D3DXMatrixTranslation
	(
		&t
		, distanceMoon * values->GlobalLight->Data.Direction.x
		, distanceMoon * values->GlobalLight->Data.Direction.y
		, distanceMoon * values->GlobalLight->Data.Direction.z
	);

	D3DXVECTOR3 cameraPos;
	values->MainCamera->Position(&cameraPos);

	D3DXMATRIX m;
	D3DXVECTOR3 rotation;
	D3DXVECTOR3 rayToCamera = values->GlobalLight->Data.Direction - cameraPos;
	rotation.x = D3DXVec3Dot(&D3DXVECTOR3(1, 0, 0), &D3DXVECTOR3(values->GlobalLight->Data.Direction.x, 0, values->GlobalLight->Data.Direction.z));
	rotation.y = D3DXVec3Dot(&D3DXVECTOR3(1, 0, 1), &D3DXVECTOR3(1, values->GlobalLight->Data.Direction.y, 1));
	D3DXMatrixRotationYawPitchRoll(&m, -phi + Math::PI / 2.0f, theta, 0);

	D3DXMATRIX cameraPosition;
	D3DXMatrixTranslation(&cameraPosition, cameraPos.x, cameraPos.y, cameraPos.z);

	moonWorldBuffer->SetMatrix(m * s * cameraPosition * t);
	moonWorldBuffer->SetVSBuffer(11);

	moon->SetShaderResource(13);
	moonShader->Render();

	blendState[1]->OMSetBlendState();
	moonRasterizer[1]->RSSetState();
	depthState[1]->OMSetDepthStencilState();
	D3D::GetDC()->Draw(6, 0);
	depthState[0]->OMSetDepthStencilState();
	moonRasterizer[0]->RSSetState();
	blendState[0]->OMSetBlendState();

}

void Sky::GenerateSphere()
{
	UINT domeCount = 32;
	UINT latitude = domeCount / 2; // 위도
	UINT longitude = domeCount; // 경도

	vertexCount = longitude * latitude * 2;
	indexCount = (longitude - 1) * (latitude - 1) * 2 * 6;

	VertexTexture* vertices = new VertexTexture[vertexCount];

	UINT index = 0;
	for (UINT i = 0; i < longitude; i++)
	{
		float xz = 100.0f * (i / (longitude - 1.0f)) * Math::PI / 180.0f;

		for (UINT j = 0; j < latitude; j++)
		{
			float y = Math::PI * j / (latitude - 1);

			vertices[index].Position.x = sinf(xz) * cosf(y);
			vertices[index].Position.y = cosf(xz);
			vertices[index].Position.z = sinf(xz) * sinf(y);
			vertices[index].Position *= 10.0f; // 크기를 키우려고 임의의 값 곱한거

			vertices[index].Uv.x = 0.5f / (float)longitude + i / (float)longitude;
			vertices[index].Uv.y = 0.5f / (float)latitude + j / (float)latitude;

			index++;
		} // for(j)
	}  // for(i)

	for (UINT i = 0; i < longitude; i++)
	{
		float xz = 100.0f * (i / (longitude - 1.0f)) * Math::PI / 180.0f;

		for (UINT j = 0; j < latitude; j++)
		{
			float y = (Math::PI * 2.0f) - (Math::PI * j / (latitude - 1));

			vertices[index].Position.x = sinf(xz) * cosf(y);
			vertices[index].Position.y = cosf(xz);
			vertices[index].Position.z = sinf(xz) * sinf(y);
			vertices[index].Position *= 10.0f; // 크기를 키우려고 임의의 값 곱한거

			vertices[index].Uv.x = 0.5f / (float)longitude + i / (float)longitude;
			vertices[index].Uv.y = 0.5f / (float)latitude + j / (float)latitude;

			index++;
		} // for(j)
	}  // for(i)


	index = 0;
	UINT* indices = new UINT[indexCount * 3];

	for (UINT i = 0; i < longitude - 1; i++)
	{
		for (UINT j = 0; j < latitude - 1; j++)
		{
			indices[index++] = i * latitude + j;
			indices[index++] = (i + 1) * latitude + j;
			indices[index++] = (i + 1) * latitude + (j + 1);

			indices[index++] = (i + 1) * latitude + (j + 1);
			indices[index++] = i * latitude + (j + 1);
			indices[index++] = i * latitude + j;
		}
	}

	UINT offset = latitude * longitude;
	for (UINT i = 0; i < longitude - 1; i++)
	{
		for (UINT j = 0; j < latitude - 1; j++)
		{
			indices[index++] = offset + i * latitude + j;
			indices[index++] = offset + (i + 1) * latitude + (j + 1);
			indices[index++] = offset + (i + 1) * latitude + j;

			indices[index++] = offset + i * latitude + (j + 1);
			indices[index++] = offset + (i + 1) * latitude + (j + 1);
			indices[index++] = offset + i * latitude + j;
		}
	}

	//CreateVertexBuffer
	{
		D3D11_BUFFER_DESC desc = { 0 };
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.ByteWidth = sizeof(VertexTexture) * vertexCount;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA data = { 0 };
		data.pSysMem = vertices;

		HRESULT hr = D3D::GetDevice()->CreateBuffer(&desc, &data, &vertexBuffer);
		assert(SUCCEEDED(hr));
	}

	//CreateIndexBuffer
	{
		D3D11_BUFFER_DESC desc = { 0 };
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.ByteWidth = sizeof(UINT) * indexCount;
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA data = { 0 };
		data.pSysMem = indices;

		HRESULT hr = D3D::GetDevice()->CreateBuffer(&desc, &data, &indexBuffer);
		assert(SUCCEEDED(hr));
	}

	SAFE_DELETE_ARRAY(vertices);
	SAFE_DELETE_ARRAY(indices);
}

void Sky::GenerateQuad()
{
	quadVertices = new VertexTexture[6];

	quadVertices[0].Position = D3DXVECTOR3(-1.0f, -1.0f, 0.0f);   //   0
	quadVertices[1].Position = D3DXVECTOR3(-1.0f, 1.0f, 0.0f);   //   1
	quadVertices[2].Position = D3DXVECTOR3(1.0f, -1.0f, 0.0f);   //   2
	quadVertices[3].Position = D3DXVECTOR3(1.0f, -1.0f, 0.0f);   //   2
	quadVertices[4].Position = D3DXVECTOR3(-1.0f, 1.0f, 0.0f);   //   1
	quadVertices[5].Position = D3DXVECTOR3(1.0f, 1.0f, 0.0f);   //   3

	quadVertices[0].Uv = D3DXVECTOR2(0, 1);   //   0
	quadVertices[1].Uv = D3DXVECTOR2(0, 0);   //   1
	quadVertices[2].Uv = D3DXVECTOR2(1, 1);   //   2
	quadVertices[3].Uv = D3DXVECTOR2(1, 1);   //   2
	quadVertices[4].Uv = D3DXVECTOR2(0, 0);   //   1
	quadVertices[5].Uv = D3DXVECTOR2(1, 0);   //   3

	//   CreateVertexBuffer
	{
		D3D11_BUFFER_DESC desc = { 0 };
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.ByteWidth = sizeof(VertexTexture) * vertexCount;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA data = { 0 };
		data.pSysMem = quadVertices;

		HRESULT hr = D3D::GetDevice()->CreateBuffer(&desc, &data, &quadBuffer);
		assert(SUCCEEDED(hr));
	}
}