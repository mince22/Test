#include "stdafx.h"
#include "MeshBunny.h"

MeshBunny::MeshBunny()
	: GameModel(Materials + L"Meshes/", L"bunny.material", Models + L"Meshes/", L"bunny.mesh")
{
	
}

MeshBunny::~MeshBunny()
{
	SAFE_DELETE(shader);
}

void MeshBunny::Update()
{
	__super::Update();
}

void MeshBunny::Render()
{
	__super::Render();
}
