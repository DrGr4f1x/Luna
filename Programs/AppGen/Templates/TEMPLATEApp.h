//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#pragma once

#include "Application.h"


class TEMPLATEApp : public Kodiak::Application
{
public:
	explicit TEMPLATEApp(const std::string& name) 
		: Application{ name }
	{}

	int ProcessCommandLine(int argc, char* argv[]) final;
	void Configure() final;
	void Startup() final;
	void Shutdown() final;

	bool Update() final;
	void Render() final;

private:
	// Add member functions here

private:
	// Add data members here
};