#include <iostream>
#include <memory>
#include <cstdio>
#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <functional>
#include <cassert>
using namespace std;

struct GroupEntry{
    GroupEntry(int id, const string& key)
        : prjId(id), groupKey(key), refCount(1) {}

    bool operator==(const GroupEntry& r) const
    {
        return prjId==r.prjId && groupKey==r.groupKey;
    }

    int prjId;
    string groupKey;
    mutable int refCount;
};

struct Hash{
    size_t operator() (const GroupEntry& r) const
    {
        string temp = to_string(r.prjId)+r.groupKey;
        hash<string> strHash;
        size_t hash = strHash(temp);
        return hash;
    }
};

class GroupIdManager;

class GroupId{
    private:
        GroupIdManager& _gim;
        unordered_set<GroupEntry, Hash>::iterator _it;
    public:
        GroupId(GroupIdManager& gim, unordered_set<GroupEntry, Hash>::iterator it)
           : _gim(gim), _it(it) {}
        ~GroupId();
        bool operator==(const GroupId& gid) const
        { return _it == gid._it; }
        int projectId(){ return _it->prjId; }
        string groupKey() { return _it->groupKey; }
};

class GroupIdManager{
    private:
        unordered_set<GroupEntry, Hash> _groupIdSet;
    public:
        GroupId getGroupId(int prjId, const string& groupKey)
        {
            auto it = _groupIdSet.find(GroupEntry(prjId, groupKey));
            if(it != _groupIdSet.end())
                it->refCount++;
            else{
                auto res = _groupIdSet.insert(GroupEntry(prjId, groupKey));
                it = res.first;
            }

            return GroupId(*this, it);
        }

        void releaseGroupId(decltype(_groupIdSet)::iterator it)
        {
            it->refCount--;
            assert(it->refCount >= 0);
            if(it->refCount == 0)       // no reference
                _groupIdSet.erase(it);
        }
};

GroupId::~GroupId() { _gim.releaseGroupId(_it); } 

int main()
{
    GroupIdManager gm;

    {
        GroupId id1 = gm.getGroupId(18, "test group");
        GroupId id2 = gm.getGroupId(18, "test group");
    }

    return 0;
}
