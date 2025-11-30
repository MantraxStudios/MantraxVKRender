#pragma once
#include "UIBehaviour.h"
#include <iostream>
#include <vector>

class UIRender
{
public:
    std::vector<UIBehaviour *> _Renders;

    void Set(UIBehaviour *render)
    {
        _Renders.push_back(render);
    }

    UIBehaviour *Get(int index)
    {
        if (index < 0 || index >= static_cast<int>(_Renders.size()))
            return nullptr;

        return _Renders[index];
    }

    const std::vector<UIBehaviour *> &GetAll() const
    {
        return _Renders;
    }

    void RenderAll()
    {
        for (auto render : _Renders)
        {
            if (render && render->isOpen)
            {
                render->OnRender();
            }
        }
    }
};
