#pragma once

class UIBehaviour
{
public:
    bool isOpen;
    virtual void OnRender() = 0;
};