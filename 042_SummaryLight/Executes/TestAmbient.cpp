#include "stdafx.h"
#include "TestAmbient.h"
#include "../Objects/MeshPlane.h"
#include "../Objects/MeshCube.h"
#include "../Objects/MeshSphere.h"
#include "../Objects/MeshBunny.h"

TestAmbient::TestAmbient(ExecuteValues * values)
	: Execute(values)
{
	shader = new Shader(Shaders + L"045_Lighting.hlsl");

	plane = new MeshPlane();
	plane->Scale(10, 1, 10);
	plane->SetShader(shader);
	plane->SetDiffuse(1, 1, 1, 1);

	cube = new MeshCube();
	cube->Scale(3, 3, 3);
	cube->Position(-10, 1.5f, 0);
	cube->SetShader(shader);
	cube->SetDiffuse(0, 1, 0, 1);

	sphere = new MeshSphere();
	sphere->Scale(3,3,3);
	sphere->Position(10, 1.5f, 0);
	sphere->SetShader(shader);
	sphere->SetDiffuse(1, 1, 1, 1);

	sphere2 = new MeshSphere();
	sphere2->Scale(3, 3, 3);
	sphere2->Position(10, 1.5f, 10);
	sphere2->SetShader(shader);
	sphere2->SetDiffuse(1, 1, 1, 1);

	bunny = new MeshBunny();
	bunny->Scale(0.02f, 0.02f, 0.02f);
	bunny->Position(0, 5, 0);
	bunny->SetShader(shader);
	bunny->SetDiffuseMap(Textures + L"White.png");
	bunny->SetDiffuse(1, 1, 1);


	{
		D3D11_BUFFER_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));

		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.ByteWidth = sizeof(LightData);

		HRESULT hr = D3D::GetDevice()->CreateBuffer(&desc, NULL, &lightBuffer);
		assert(SUCCEEDED(hr));
	}

}

TestAmbient::~TestAmbient()
{
	SAFE_DELETE(plane);
	SAFE_DELETE(cube);
	SAFE_DELETE(sphere);
	SAFE_DELETE(sphere2);
	SAFE_DELETE(bunny);

	SAFE_DELETE(shader);
	SAFE_DELETE(lightBuffer);
}

void TestAmbient::Update()
{
	plane->Update();
	sphere->Update();
	cube->Update();
	sphere2->Update();
	bunny->Update();
}

void TestAmbient::PreRender()
{

}

void TestAmbient::Render()
{

	plane->Render();
	cube->Render();
	sphere->Render();
	sphere2->Render();
	bunny->Render();
}

void TestAmbient::PostRender()
{
	
}


