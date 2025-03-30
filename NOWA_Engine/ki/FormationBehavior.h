#ifndef FORMATION_BEHAVIOR_H
#define FORMATION_BEHAVIOR_H

#include "defines.h"
#include "Path.h"
#include "utilities/LuaObserver.h"

namespace NOWA
{
	class GameObject;
	class PhysicsActiveComponent;
	class IAnimationBlender;
	class CrowdComponent;

	typedef boost::shared_ptr<GameObject> GameObjectPtr;

	namespace KI
	{
		class EXPORTED FormationBehavior
		{
		public:
			enum FormationType
			{
				NONE = 1 << 0,
				LINE = 1 << 1,
				WEDGE = 1 << 2,
				CIRCLE = 1 << 3,
				SQUARE = 1 << 4,
				COLUMN = 1 << 5,
				ECHELON_LEFT = 1 << 6,
				ECHELON_RIGHT = 1 << 7
			};
		public:
			FormationBehavior();

			virtual ~FormationBehavior();

            void setLeader(PhysicsActiveComponent* leader);

            PhysicsActiveComponent* getLeader() const;
           
            void setFormationAgents(const std::vector<unsigned long>& formationAgentIds);

            void addFormationAgent(unsigned long agentId);

            void removeFormationAgent(unsigned long agentId);

            void setFormationType(FormationType formationType);

            void setFormationType(const Ogre::String& formationTypeStr);

            FormationType getFormationType() const;

            void setSpacing(Ogre::Real spacing);

            Ogre::Real getSpacing() const;

            void setAutoOrientation(bool autoOrient);

            bool getAutoOrientation() const;

            void setFlyMode(bool flyMode);

            bool getFlyMode() const;

            FormationType mapFormationType(const Ogre::String& formationTypeStr);

            Ogre::String formationTypeToString(FormationType formationType);

            void update(Ogre::Real dt);

        private:
			Ogre::Vector3 calculateFormationOffset(size_t agentIndex, const Ogre::Vector3& forward, const Ogre::Vector3& right, const Ogre::Vector3& up);

			void applyMovement(PhysicsActiveComponent* agent, const Ogre::Vector3& resultVelocity, Ogre::Real dt);

		private:
			PhysicsActiveComponent* leader;
			std::vector<PhysicsActiveComponent*> formationAgents;
			FormationType currentFormationType;
			Ogre::Real spacing;
			bool autoOrientation;
			bool flyMode;
		};

	}; //end namespace KI

}; //end namespace NOWA

#endif