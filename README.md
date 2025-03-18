# Arcade Engine
Arcade Engine is an engine written in C++, using the following libraries:
* [SFML](https://sfml-dev.org)
* [Dear ImGui](https://github.com/ocornut/imgui)
* [ImGui-SFML](https://github.com/SFML/imgui-sfml)
* [Box2D](https://box2d.org/)
  
#### Table of Contents
1) [The Bind Manager](#the-bind-manager)
2) [The Event Manager](#the-event-manager)
   1) [Understanding the Poll Order](#understanding-event-poll-order)
   2) [Registering Events](#registering-events)
3) [The `.alv` file format](#the-alv-file-format)

### The Bind Manager
Arcade Engine provides `Arcade::BindManager`, which is a class responsible for binding both `sf::Keyboard::Key`s and `sf::Mouse::Button`s to an `std::string` for easy lookup. You can use it easily with the following functions:
> `bool Arcade::BindManager::bind( std::string, sf::Keyboard::Key )`
> <br>&emsp;&emsp;&emsp;Creates a new `Arcade::Bind` indexed by the provided string that will active when the matching key is pressed
> <br>&emsp;&emsp;&emsp;Returns whether or not it succeeded in making the bind. Will return false if the name is already in use.

> `bool Arcade::BindManager::bind( std::string, sf::Mouse::Button )`
> <br>&emsp;&emsp;&emsp;Same as previous, except actives for a mouse button instead

> `bool Arcade::BindManager::unbind( std::string )`
> <br>&emsp;&emsp;&emsp;Removes a bind. Returns false if it does not exist.

> `const Bind* getBind( std::string )`
> <br>&emsp;&emsp;&emsp;Returns the bind indexed by the string, or `nullptr` if it does not exist. It is not recommended you use this most of the time. Instead use one of the access functions.

> `std::unordered_map<std::string, Bind> getAllBinds()`
> <br>&emsp;&emsp;&emsp;Returns a copy of the internal binding map. This is mostly useful for checking if a bind exists, or debugging.

> `bool isPressed( std::string )`<br>
> `bool startedPressing( std::string )`<br>
> `bool endedPressing( std::string )`
> <br>&emsp;&emsp;&emsp;All 3 of these functions do what they say on the tin. They return if the bind is pressed this frame, if it was just started pressing, or if the player just let go. They all return false if the bind does not exist.

> `sf::Time getTimePressed( std::string )`
> <br>&emsp;&emsp;&emsp;Returns the `sf::Time` representation of how long the button has been pressed. Returns `sf::Time::Zero` if it does not exist.

### The Event Manager
Arcade does not directly provide an event manager, rather the event manager is directly part of `Arcade::Engine`, so all events are simply attached to that.
#### Understanding Event Poll Order
There are two types of events in Arcade. Window-Events, and Game-Events. Window-Events are all provided by the `sf::Event` interface, and are all called at the beginning of an update loop by the engine. These cannot be called externally. Window-Events do not have a specific call order. For more information, see [`sf:Window::pollEvent()`](https://www.sfml-dev.org/documentation/3.0.0/classsf_1_1WindowBase.html#a6090926b477e9d0a83854b94b9e1fd35).
Game-Events are provided by the engine, Here are the different kinds. @todo
* [`PhysicsCollide`]() `PhysBody a, PhysBody b, sf::Vector2f force`
  > The `PhysicsCollide` event is called when two bodies collide. This is called during the physics section of the [update loop](#the-update-loop).
* [`EntityDamaged`]() `Entity ent, Damage dmg`
	> Called when an entity takes damage. See documentation on the passed classes for more info.
* [`LevelLoaded`]()
	> Called when the level is fully loaded. Attempting to perform certain actions before this may cause issues. You can also check `Engine:isLevelLoaded()`
* [`EntitySpawned`]() `Entity ent`
	> Called whenever an entity is created. Please note that this is not called for map entities, only entities created after map load.
* [`LuaEvent`]() `std::string event_name, LuaArgs...`
	> Mostly used internally for mod events. You typically won't need to bind to this in C++. See the lua docs for more information on how to use this.

#### Registering Events
> `bool Arcade::Engine::registerEvent<EVENT_T>( std::string, std::function<void(EVENT_T)>)`
> <br>&emsp;&emsp;&emsp;Registers the callback to the event provided by the template. This event can be called an infinite number of times. Events are registered by strings, allowing them to be re-referenced later. Returns whether or not event creation succeeded. Example usage:
> <br>&emsp;&emsp;&emsp;`engine.registerEvent<sf::Event::Resize>("myResizeHandler", [](auto e) { /* ... */ })`

> `bool Arcade::Engine::registerEventOnce<EVENT_T>( std::string, std::function<void(EVENT_T)>)`
> <br>&emsp;&emsp;&emsp;Registers the callback to the event provided by the template. However, after a single call, the event will be discarded.

> `bool Arcade::Engine::deregisterEvent( std::string )`
> <br>&emsp;&emsp;&emsp;Removes the event if it exists.

### The `.alv` file format
`.alv` files are how Arcade engine stores level data. Level data is comprised of many parts, and thus it is quite a complex file format. All .alv files are **big endian**.

---

> **[4 Bytes] Magic number**
> <br>&emsp;&emsp;&emsp;The Magic number is always the same `0x00414C56`, or `\0ALV`. This is used to identify a file as an actual alv file and not just a file with the `.alv` extension.

> **[3+ Bytes] Level Name**
> <br>&emsp;&emsp;&emsp; Level name as a translation key. ie: `"prologue.level1"`. Level name is always terminated by `0x00`. If the string exceeds 64 characters, or is less than 3, the level loader will throw an `std::length_error`.

> **[8 Bytes] World Size**
> <br>&emsp;&emsp;&emsp;A pair of `uint_32t` that represent the size of the level in chunks. A world must contain at least one chunk.

> **[8 Bytes] Chunk size**
> <br>&emsp;&emsp;&emsp;The size of the tile chunks on all layers as a 64-bit number. This must be > 0. 1 tile chunks are technically possible but not recommended. This value is the width of the chunk. Chunks are squares, so the total tile number is $n^2$

#### <u>\_Repeats for Each Chunk\_</u>

> **[ 1 Byte ] Chunk Header**
> <br>&emsp;&emsp;&emsp; Chunk header data for setting chunk flags. Here are the bits from top to bottom:
> > `Chunk::Flag::HasTiles = 0b1000000`
> > <br>&emsp;&emsp;&emsp;This flag is `1` if the chunk contains tiles. If this is `0` we skip this chunk.
> > <br>&emsp;&emsp;&emsp;`(flag >> 7) & 1`
>
> > `Chunk::Flag::DeadChunk = 0b01000000`
> > <br>&emsp;&emsp;&emsp;`1` if this check is 'dead'. A dead chunk is not in memory at all, and it is stored as a `nullptr`. This is rarely used, but it can save memory on very large maps with oddly shaped layouts. Typically though you should just set `HasTiles` to `0`
>
> > `Chunk::Flag::OoB = 0b00100000`
> > <br>&emsp;&emsp;&emsp;Marks a chunk as 'Out of Bounds'. If an entity ends up here, it will get teleported to `lastStablePosition()`.
> 
> > `Chunk::Flag::NoCull = 0b00010000`
> > <br>&emsp;&emsp;&emsp;Tells the engine never to cull this chunk. Usefully for `RenderCamera`, when you need the contents of a chunk to be rendered onto a texture.

> **[3+ Bytes] Chunk Dictionary**
> <br>&emsp;&emsp;&emsp;Is also skipped if `Chunk::Flag::HasTiles` = `0`. Is a dictionary mapping tile numeric IDs to internal tile string identifiers. The first value is a byte containing the number of entries. The chunk dictionary is limited to 256 entries, which is more than enough for a chunk. If you need more, get help. Each entry is an 8-bit number, followed by a C-string. 0 is a valid index, so do not confuse `\0` with `0x00`.

> **[\* Bytes] Tile data**
> > **[1 Byte] Tile Header**
> > <br>&emsp;&emsp;&emsp;Works like chunk header. Data for tile compression and metadata.
> > > `Tile::Flag::Empty = 0b10000000`
> > > <br>&emsp;&emsp;&emsp;Flags this tile to be skipped.
> >
> > > `Tile::Flag::UsesNumericID = 0b01000000`
> > > <br>&emsp;&emsp;&emsp;This should not be used 90% of the time. Using numeric IDs is smaller, but runs the risk of an ID change causing bad mapping. 
> >
> > > `Tile::Flag::HasData = 0b00100000`
> > > <br>&emsp;&emsp;&emsp;Indicates that this tile is preceded by a tile data entry. If `0` then we skip the Tile data process. 
> >
> > > `Tile::Flag::Ghost = 0b00010000`
> > > <br>&emsp;&emsp;&emsp;Marks a "ghost" tile. Ghost tiles are visual only, meaning they only calculate lightning. Nothing else is computed
> >
> > > `Tile::Flag::ExcludeCollisionMerge = 0b00001000`
> > > <br>&emsp;&emsp;&emsp;Excludes a tile from having its CollisionShape merged with the rest in the chunk.
>
> > **[1+ Byte] Data**
> > <br>&emsp;&emsp;&emsp;This is skipped entirely if `Tile::Flag::HasData` is `0`. Data is at maximum 255 entries. Tile data includes things like a reference name. Data is stored as `DATA_CODE DATA_VALUE`. Data code is an 8-bit number. See `Tile::DataCode` default values below.
> > > `Tile::DataCode::RefName = 0x00`
> > > <br>&emsp;&emsp;&emsp;An attachable reference name. Looks until the next null character (`0x00`). Example:
> > ><br>&emsp;&emsp;&emsp;`0x00 4D 79 54 69 6C 65 00` **=>** `RefName: "MyTile"`
> >
> > > `Tile::DataCode::CollisionShape 0x01`
> > > <br>&emsp;&emsp;&emsp;Allows the specification of a custom collision shape. Format is the number of points, followed by two unsigned 4-bit numbers. There must be at least 3 points, and they will wrap clockwise into a collision polygon, which is then used to compute the chunk's collision mesh. (x,y) is measured from the top left. Example:
> > > <br>&emsp;&emsp;&emsp;`0x01 03 0F F0 FF` **=>** `CollisionShape{(0.0,1.0), (1.0,0.0), (1.0,1.0)}`
>
> > **[1 Bytes] Tile**
> > <br>&emsp;&emsp;&emsp;An ID that indexes into the chunks map.

---