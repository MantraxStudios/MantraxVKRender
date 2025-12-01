#pragma once
#include "IService.h"
#include "UIBehaviour.h"
#include <vector>

class UIRender : public IService
{
public:
    std::vector<UIBehaviour *> _Renders;

    std::string getName() override
    {
        return "UIRender";
    }

    void Set(UIBehaviour *render)
    {
        _Renders.push_back(render);
    }

    UIBehaviour *Get(int index)
    {
        if (index < 0 || index >= (int)_Renders.size())
            return nullptr;
        return _Renders[index];
    }

    template <typename T>
    T *GetByType()
    {
        for (auto *r : _Renders)
        {
            if (auto casted = dynamic_cast<T *>(r))
                return casted;
        }
        return nullptr;
    }

    const std::vector<UIBehaviour *> &GetAll() const
    {
        return _Renders;
    }

    void RenderAll()
    {
        for (auto &r : _Renders)
        {
            if (r && r->isOpen)
                r->OnRender();
        }
    }
};
