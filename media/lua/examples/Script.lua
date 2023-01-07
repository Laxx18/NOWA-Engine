-- Lua script for test program.

print( 'Hello from LUA!' )

app = getApplication()

app:setBackgroundColour( ColourValue( 0, 0, .25 ) )

camera = app:getCamera()

camera:setPosition( Vector3(0,100,500) )

camera:lookAt( Vector3(0,0,0) )

camera:setNearClipDistance( 5 )

e = app:createEntity( 'Ninja', 'ninja.mesh' )

e:setDisplaySkeleton( true )

rootNode = app:getRootNode()

child = rootNode:createChildSceneNode( 'NinjaNode' )

child:attachObject( e )

child:yaw( 180 )

child:setPosition( Vector3.ZERO )

e = app:createEntity( 'floor', 'ground' )

e:setMaterialName( 'Examples/Rockwall' )

child = rootNode:createChildSceneNode( 'FloorNode' )

child:attachObject( e )

print( 'End of File Script' )

