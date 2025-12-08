#ifndef MANTRAX_EXPORT_H
#define MANTRAX_EXPORT_H

// ✅ SOLUCIÓN TEMPORAL: Forzar exports si estamos compilando el DLL
// (Elimina esto una vez que configures MANTRAX_EXPORTS en las propiedades del proyecto)
#if defined(_WINDLL) && !defined(MANTRAX_EXPORTS)
#define MANTRAX_EXPORTS
#endif

#ifdef _WIN32
#ifdef MANTRAX_EXPORTS
#define MANTRAX_API __declspec(dllexport)
#pragma message("✅ Compilando DLL con dllexport")
#else
#define MANTRAX_API __declspec(dllimport)
#pragma message("ℹ️  Importando DLL con dllimport")
#endif
#else
#define MANTRAX_API
#endif

#endif // MANTRAX_EXPORT_H