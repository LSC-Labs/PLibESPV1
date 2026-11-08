#pragma once
/**
 * @file Registration.h
 * @brief Automatic unregistration of objects from registries.
 *
 * Problem: Registries (message bus, config/status handler lists, HA component
 * table) store raw pointers. If a registered object is destroyed, the registry
 * keeps a dangling pointer and the next call crashes.
 *
 * Solution:
 *  - A registry implements IRegistry.
 *  - A registrable interface owns a CRegistrations member. When the registry
 *    stores the object, it adds itself to that list (attach()).
 *  - When the object is destroyed, the interface destructor calls
 *    CRegistrations::releaseAll(this) and every registry removes the object.
 *  - When a registry is destroyed first, it calls detach() on all registered
 *    objects, so they do not call a destroyed registry later.
 *
 * Note: The base class destructor runs after the destructor of the derived
 * class. The object stays registered while the derived destructor body runs -
 * do not send events or trigger registry calls there that reach this object.
 */
#include <vector>

/**
 * @brief Interface of a registry that stores pointers to other objects.
 */
class IRegistry {
    public:
        /**
         * @brief Called while a registered object is destroyed.
         * Remove every entry of this object - do not call any of its methods.
         * @param pObject Address of the registered interface (as stored).
         */
        virtual void forgetRegisteredObject(const void *pObject) = 0;

    protected:
        ~IRegistry() {}
};

/**
 * @brief List of registries an object is registered in.
 *
 * Copying an object does not copy its registrations (the copy is not registered).
 */
class CRegistrations {
    private:
        std::vector<IRegistry *> m_tRegistries;

    public:
        CRegistrations() {}
        CRegistrations(const CRegistrations &) {}
        CRegistrations & operator=(const CRegistrations &) { return(*this); }

        /// @brief Remember that the object is stored in pRegistry.
        void attach(IRegistry *pRegistry) {
            if(!pRegistry) return;
            for(IRegistry *pEntry : m_tRegistries) if(pEntry == pRegistry) return;
            m_tRegistries.push_back(pRegistry);
        }

        /// @brief Forget pRegistry (object removed from it, or registry destroyed).
        void detach(IRegistry *pRegistry) {
            for(size_t nIdx = 0; nIdx < m_tRegistries.size(); ) {
                if(m_tRegistries[nIdx] == pRegistry) m_tRegistries.erase(m_tRegistries.begin() + nIdx);
                else nIdx++;
            }
        }

        /// @brief true if the object is stored in pRegistry.
        bool isAttached(const IRegistry *pRegistry) const {
            for(IRegistry *pEntry : m_tRegistries) if(pEntry == pRegistry) return(true);
            return(false);
        }

        /// @brief Number of registries the object is stored in.
        size_t count() const { return(m_tRegistries.size()); }

        /// @brief Remove the object from all registries (call in the destructor of the interface).
        void releaseAll(const void *pObject) {
            std::vector<IRegistry *> tRegistries;
            tRegistries.swap(m_tRegistries);
            for(IRegistry *pRegistry : tRegistries) pRegistry->forgetRegisteredObject(pObject);
        }
};
