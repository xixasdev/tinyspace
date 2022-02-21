// saveable.cpp (tiny space v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.12 | LICENSE: WTFPL/PDM/CC0... your choice


#include "saveable.hpp"


namespace tinyspace {


std::atomic<bool> isSaving( false );
std::atomic<std::thread*> saveThread( nullptr );


bool isSavingThread()
{
    if ( ! isSaving )
    {
        return false;
    }
    std::thread* saveThreadPtr = saveThread.load();
    if ( ! saveThreadPtr)
    {
        return false;
    }
    return saveThreadPtr->get_id() == std::this_thread::get_id();
}

namespace {
    std::unordered_set<Updateable*> updateables_aftersave;
}


Updateable::Updateable()
{
    updateables_aftersave.insert( this );
}


Updateable::~Updateable()
{
    updateables_aftersave.erase( this );
}


void update_aftersave()
{
    for (auto u : updateables_aftersave)
    {
        u->update();
    }
}


} //tinyspace
