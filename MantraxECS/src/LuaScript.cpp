#include "../include/LuaScript.h"

LuaScript::LuaScript(const std::string &scriptPath)
    : scriptPath_(scriptPath)
{
    lua_.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::table,
        sol::lib::package);

    loadFromFile(scriptPath_);
}

void LuaScript::reload()
{
    loadFromFile(scriptPath_);
}

void LuaScript::loadFromFile(const std::string &path)
{
    sol::load_result loaded = lua_.load_file(path);

    if (!loaded.valid())
    {
        sol::error err = loaded;
        throw std::runtime_error(
            std::string("Error al cargar script Lua desde '") +
            path + "': " + err.what());
    }

    // Luego lo ejecutamos
    sol::protected_function_result exec_result = loaded();

    if (!exec_result.valid())
    {
        sol::error err = exec_result;
        throw std::runtime_error(
            std::string("Error al ejecutar script Lua '") +
            path + "': " + err.what());
    }
}

int LuaScript::callIntFunction(const std::string &funcName, int arg)
{
    sol::object obj = lua_[funcName];

    if (!obj.is<sol::protected_function>())
    {
        throw std::runtime_error(
            "La función Lua '" + funcName + "' no existe o no es callable");
    }

    sol::protected_function func = obj.as<sol::protected_function>();

    sol::protected_function_result result = func(arg);

    if (!result.valid())
    {
        sol::error err = result;
        throw std::runtime_error(
            std::string("Error al llamar a la función Lua '") +
            funcName + "': " + err.what());
    }

    try
    {
        return result.get<int>();
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(
            std::string("La función Lua '") + funcName +
            "' no devolvió un int válido: " + e.what());
    }
}

sol::state &LuaScript::state()
{
    return lua_;
}
