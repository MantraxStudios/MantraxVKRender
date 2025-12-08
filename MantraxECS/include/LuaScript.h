#pragma once

#include <string>
#include <stdexcept>
#include <sol/sol.hpp>
#include "EngineLoaderDLL.h"

class MANTRAX_API LuaScript
{
public:
    /// Carga y ejecuta el script Lua desde un archivo.
    explicit LuaScript(const std::string &scriptPath);

    /// Recarga el script desde el archivo original.
    void reload();

    /// Asigna una variable global accesible desde Lua: lua[name] = value
    template <typename T>
    void setGlobal(const std::string &name, T &&value)
    {
        lua_[name] = std::forward<T>(value);
    }

    /// Llama una función de Lua que recibe un int y devuelve un int.
    /// Lanza std::runtime_error si falla algo.
    int callIntFunction(const std::string &funcName, int arg);

    /// Acceso directo al estado Lua por si quieres hacer cosas avanzadas.
    sol::state &state();

private:
    void loadFromFile(const std::string &path);

private:
    sol::state lua_;
    std::string scriptPath_;
};
