#pragma once
#include "../UIBehaviour.h"
#include <iostream>

class SceneView : public UIBehaviour
{
public:
    SceneView()
    {
        isOpen = true;
    }

    void OnRender() override;
};
