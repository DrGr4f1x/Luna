//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

struct DummyPayLoad
{
    uint dummyData;
};

// We don't use pay loads in this sample, but the fn call requires one
groupshared DummyPayLoad dummyPayLoad;

[numthreads(1, 1, 1)]
void main()
{
    DispatchMesh(3, 1, 1, dummyPayLoad);
}