#ifndef _UTIL_H
#define _UTIL_H

#include <QList>
#include <QStringList>
#include <QVariant>

struct lua_State;

template <typename T>
void qShuffle(QList<T> &list)
{
    for (int i = 0, n = list.length(); i < n; ++i) {
        int r = qrand() % (n - i) + i;
        list.swap(i, r);
    }
}

// lua interpreter related
lua_State *CreateLuaState();
bool DoLuaScript(lua_State *L, const char *script);

QVariant GetValueFromLuaState(lua_State *L, const char *table_name, const char *key);

QStringList IntList2StringList(const QList<int> &intlist);
QList<int> StringList2IntList(const QStringList &stringlist);
QVariantList IntList2VariantList(const QList<int> &intlist);
QList<int> VariantList2IntList(const QVariantList &variantlist);

bool isNormalGameMode(const QString &mode);

static const int S_EQUIP_AREA_LENGTH = 5;

#endif