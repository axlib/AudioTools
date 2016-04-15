//
//  atUniqueNameComponent.hpp
//  AudioTools
//
//  Created by Alexandre Arsenault on 2016-04-15.
//  Copyright © 2016 Alexandre Arsenault. All rights reserved.
//

#ifndef atUniqueNameComponent_hpp
#define atUniqueNameComponent_hpp

#include <OpenAX/Utils.h>
#include <OpenAX/Window.h>

namespace at {
	class UniqueNameComponent : public ax::Component {
	public:
		/// Shared pointer.
		typedef std::shared_ptr<UniqueNameComponent> Ptr;
		
		UniqueNameComponent(ax::Window* win);
		
		virtual ~UniqueNameComponent();
		
		ax::Window* GetWindow();
		
		void SetName(const std::string& name);
		
		std::string GetName() const;
		
	protected:
		ax::Window* _win;
		std::string _name;
	};
}

#endif /* atUniqueNameComponent_hpp */
