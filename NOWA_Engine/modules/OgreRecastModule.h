#ifndef OGRE_RECAST_MODULE_H
#define OGRE_RECAST_MODULE_H

#include "OgreRecastDefinitions.h"
#include "OgreRecast.h"
#include "OgreDetourCrowd.h"
#include "OgreDetourTileCache.h"

#include "main/Events.h"
#include "defines.h"

namespace NOWA
{
	class GameObject;

	class EXPORTED OgreRecastModule
	{
	public:
		friend class AppState; // Only AppState may create this class

		/**
         * Creates and initializes an OgreRecast navigation mesh instance.
         *
         * @param[in] sceneManager   The Ogre SceneManager that owns the geometry.
         * @param[in] params         Navigation mesh configuration. See parameter details below.
         * @param[in] pointExtends   3D half-size of the search box used to snap world positions
         *                           onto the navmesh. See detailed explanation below.
         *
         * ============================================================
         * ENGLISH — PARAMETER REFERENCE
         * ============================================================
         *
         * ── VOXEL RESOLUTION ─────────────────────────────────────────
         *
         * cellSize  (cs)
         *   The horizontal size of one voxel in world units. Think of it as the
         *   "pixel size" of your navmesh on the ground plane. Smaller values produce
         *   a more accurate navmesh that follows wall edges tightly, but increase
         *   build time and memory use.
         *
         *   Rule of thumb: aim for at least 10 voxels across the narrowest corridor.
         *     corridor = 2m, cellSize = 0.2  ->  10 voxels across  ✓
         *     corridor = 2m, cellSize = 0.5  ->   4 voxels across  ⚠ too coarse
         *
         *   Minimum allowed value is approximately 0.05 (floating-point precision limit).
         *   [Limit: > 0] [Units: world units]
         *
         * cellHeight  (ch)
         *   The vertical size of one voxel. On flat floors this can equal cellSize.
         *   Lower values improve precision for thin steps, ramps, and low ceilings,
         *   without significantly increasing memory use.
         *   Minimum allowed value is approximately 0.05.
         *   [Limit: > 0] [Units: world units]
         *
         * ── AGENT (CHARACTER) SETTINGS ───────────────────────────────
         *
         * agentHeight
         *   Height of the player character in world units. Recast marks any space
         *   shorter than this value as unwalkable — useful for low ceilings and
         *   overhangs. Precision is determined by cellHeight.
         *
         * agentRadius  ← THE MOST CRITICAL PARAMETER
         *   The radius of the character's capsule on the ground plane. Recast erodes
         *   (shrinks) the walkable area inward by this value on ALL sides, preventing
         *   the character from walking into walls.
         *
         *   If agentRadius is too large relative to corridor width, the entire
         *   corridor is erased from the navmesh and pathfinding fails with error -3!
         *
         *   Walkable width after erosion:
         *     walkable = corridorWidth - (2 × agentRadius)
         *
         *     2m corridor, radius 0.3  ->  2.0 - 0.6 = 1.4m walkable  ✓
         *     2m corridor, radius 0.8  ->  2.0 - 1.6 = 0.4m walkable  ⚠ very thin
         *     2m corridor, radius 1.1  ->  2.0 - 2.2 = negative       ✗ corridor gone!
         *
         *   Recommended for a 2m corridor: 0.3
         *   [Limit: >= 0] [Units: world units]
         *
         * agentMaxClimb
         *   Maximum step height the character can traverse (like a stair riser).
         *   On flat labyrinth floors keep this small (e.g. 0.2). If set too high,
         *   Recast may bridge areas that should be separated by walls.
         *   Precision is determined by cellHeight.
         *   [Limit: >= 0] [Units: world units]
         *
         * agentMaxSlope
         *   Maximum walkable slope in degrees. On a flat floor set this very low
         *   (e.g. 5 degrees). Steeper surfaces become unwalkable.
         *   [Limit: 0 <= value < 90] [Units: degrees]
         *
         * ── REGION SETTINGS ──────────────────────────────────────────
         *
         * regionMinSize
         *   Minimum area (in voxels²) that an isolated navmesh island must have to
         *   survive. Islands smaller than this are discarded as noise (table tops,
         *   tiny ledges, etc.). In a labyrinth keep this small so corridor corners
         *   and dead ends are not accidentally removed.
         *   Internally this value is squared: minRegionArea = regionMinSize².
         *   [Limit: >= 0] [Units: voxels]
         *
         * regionMergeSize
         *   Small adjacent regions up to this size are merged together into larger
         *   polygons. Larger values produce fewer, bigger polygons (faster pathfinding,
         *   less accurate shape). Keep moderate for narrow corridors.
         *   Internally: mergeRegionArea = regionMergeSize².
         *   [Limit: >= 0] [Units: voxels]
         *
         * ── EDGE SETTINGS ────────────────────────────────────────────
         *
         * edgeMaxLen
         *   Maximum length of a navmesh polygon edge. Extra vertices are inserted to
         *   keep edges below this limit. Shorter = more polygons but edges follow
         *   walls more precisely. A value of 0 disables the limit entirely.
         *   Precision is affected by cellSize.
         *   [Limit: >= 0] [Units: world units]
         *
         * edgeMaxError  (maxSimplificationError / edgeMaxDeviation)
         *   Maximum deviation of a simplified contour edge from the original voxel
         *   contour in the xz plane. Smaller = edges follow walls more precisely
         *   but more triangles are generated. A value of 0 is not recommended as
         *   it can cause a large triangle count explosion.
         *   [Limit: >= 0] [Units: world units]
         *
         * ── POLYGON SETTINGS ─────────────────────────────────────────
         *
         * vertsPerPoly
         *   Maximum number of vertices per navmesh polygon. Detour's hard limit is
         *   DT_VERTS_PER_POLYGON = 6. Higher values produce better-formed polygons
         *   at a small extra processing cost. Always use 6.
         *   [Limit: >= 3, <= 6 for Detour]
         *
         * detailSampleDist  (contourSampleDistance)
         *   Sampling distance when building the height detail mesh. Controls how
         *   closely the detail mesh follows the source geometry surface. Not critical
         *   for flat floors — set to corridor width or larger. Values below 0.9
         *   disable this feature entirely.
         *   [Limit: 0 or >= 0.9] [Units: world units]
         *
         * detailSampleMaxError  (contourMaxDeviation)
         *   Maximum deviation of the detail mesh surface from the source geometry
         *   height field. Not critical for flat floors. A value of 0 is not
         *   recommended as it can produce a large triangle count.
         *   [Limit: >= 0] [Units: world units]
         *
         * keepInterResults
         *   If true, intermediate build structures (heightfield, compact heightfield,
         *   contour set) are kept in memory after navmesh creation. Useful for
         *   debug rendering. Set to false in production to free memory.
         *
         * ── NAVMESH QUERY NODE POOL ──────────────────────────────────
         *
         * m_navQuery->init(m_navMesh, nodePoolSize)
         *   This is NOT a config param but is critical for deep labyrinths.
         *   The node pool is the internal A* search buffer. In a complex labyrinth
         *   the pathfinder must explore many polygons before reaching the target.
         *   If the pool is too small (default 2048), A* runs out of nodes mid-search
         *   and returns DT_FAILURE — causing error -3 even when a valid path exists.
         *
         *   Always use 65535 for labyrinths:
         *     m_navQuery->init(m_navMesh, 65535);
         *
         * ── POINTEXTENDS (RUNTIME SNAP SEARCH BOX) ───────────────────
         *
         * pointExtends  (mExtents)
         *   A 3D half-size search box used by findNearestPoly() when snapping a
         *   world-space position (click target, agent start) to the nearest navmesh
         *   polygon. Detour searches within ±extents around the given point.
         *
         *     mExtents[0] = X half-width  (left/right)
         *     mExtents[1] = Y half-height (up/down)
         *     mExtents[2] = Z half-depth  (forward/back)
         *
         *   Think of it as a magnet radius — larger means it reaches further to find
         *   a polygon. The DANGER in a labyrinth:
         *
         *     Wall
         *     |  Corridor A  |##wall##|  Corridor B  |
         *           ↑ click here                ↑ snapped here if extents too large!
         *
         *   If X/Z extents exceed half the corridor width, the snap can reach THROUGH
         *   a wall into the adjacent corridor. Detour then tries to path between two
         *   unconnected polygons and returns error -3.
         *
         *   Rule of thumb:
         *     pointExtends.x < corridorWidth / 2
         *     pointExtends.z < corridorWidth / 2
         *     pointExtends.y can be more generous (for multi-floor, height variation)
         *
         *   Recommended for a flat 2m corridor labyrinth:
         *     pointExtends = Vector3(0.8f, 1.0f, 0.8f)
         *
         * ── RECOMMENDED SETTINGS FOR A FLAT 2m CORRIDOR LABYRINTH ────
         *
         *   cellSize             = 0.2f    // 10 voxels per 2m corridor
         *   cellHeight           = 0.2f    // flat floor
         *   agentHeight          = 1.8f    // character height
         *   agentRadius          = 0.3f    // leaves 1.4m walkable after erosion
         *   agentMaxClimb        = 0.2f    // flat floor, keep small
         *   agentMaxSlope        = 5.0f    // nearly flat
         *   regionMinSize        = 4
         *   regionMergeSize      = 8
         *   edgeMaxLen           = 4.0f
         *   edgeMaxError         = 1.0f
         *   vertsPerPoly         = 6
         *   detailSampleDist     = 2.0f
         *   detailSampleMaxError = 0.1f
         *   pointExtends         = Vector3(0.8f, 1.0f, 0.8f)
         *   navQuery node pool   = 65535
         *
         *
         * ============================================================
         * DEUTSCH — PARAMETER-REFERENZ
         * ============================================================
         *
         * ── VOXEL-AUFLÖSUNG ──────────────────────────────────────────
         *
         * cellSize  (cs)
         *   Die horizontale Größe eines Voxels in Welteinheiten. Stell es dir wie
         *   die "Pixelgröße" deines Navmesh vor. Kleinere Werte erzeugen ein
         *   genaueres Navmesh, das Wandkanten enger folgt, erhöhen aber Bauzeit und
         *   Speicherverbrauch.
         *
         *   Faustregel: mindestens 10 Voxel über den schmalsten Korridor.
         *     Korridor = 2m, cellSize = 0.2  ->  10 Voxel Breite  ✓
         *     Korridor = 2m, cellSize = 0.5  ->   4 Voxel Breite  ⚠ zu grob
         *
         *   Mindestwert liegt bei ca. 0.05 (Gleitkomma-Präzisionsgrenze).
         *   [Limit: > 0] [Einheit: Welteinheiten]
         *
         * cellHeight  (ch)
         *   Die vertikale Größe eines Voxels. Bei flachen Böden kann dieser Wert
         *   gleich cellSize sein. Kleinere Werte verbessern die Erkennung von
         *   flachen Stufen, Rampen und niedrigen Decken, ohne den Speicherverbrauch
         *   wesentlich zu erhöhen.
         *   Mindestwert liegt bei ca. 0.05.
         *   [Limit: > 0] [Einheit: Welteinheiten]
         *
         * ── AGENTEN-EINSTELLUNGEN ────────────────────────────────────
         *
         * agentHeight
         *   Höhe des Spielercharakters in Welteinheiten. Recast markiert jeden
         *   Bereich kleiner als dieser Wert als nicht begehbar — nützlich für
         *   niedrige Decken und Überhänge. Präzision wird durch cellHeight bestimmt.
         *
         * agentRadius  ← DER WICHTIGSTE PARAMETER ÜBERHAUPT
         *   Der Radius der Charakter-Kapsel auf der Bodenebene. Recast erodiert
         *   (schrumpft) die begehbare Fläche von ALLEN Seiten um diesen Wert, um
         *   zu verhindern, dass der Charakter in Wände läuft.
         *
         *   Ist agentRadius im Verhältnis zur Korridorbreite zu groß, wird der
         *   gesamte Korridor aus dem Navmesh gelöscht — Wegfindung schlägt mit
         *   Fehler -3 fehl!
         *
         *   Begehbare Breite nach Erosion:
         *     begehbar = Korridorbreite - (2 × agentRadius)
         *
         *     2m Korridor, Radius 0.3  ->  2.0 - 0.6 = 1.4m begehbar  ✓
         *     2m Korridor, Radius 0.8  ->  2.0 - 1.6 = 0.4m begehbar  ⚠ sehr schmal
         *     2m Korridor, Radius 1.1  ->  2.0 - 2.2 = negativ        ✗ Korridor weg!
         *
         *   Empfehlung für 2m Korridor: 0.3
         *   [Limit: >= 0] [Einheit: Welteinheiten]
         *
         * agentMaxClimb
         *   Maximale Stufenhöhe, die der Charakter in einem Schritt überwinden kann
         *   (wie eine Treppenstufe). Bei flachen Labyrinthböden klein halten (z.B.
         *   0.2). Zu großer Wert kann Bereiche verbinden, die durch Wände getrennt
         *   sein sollten.
         *   Präzision wird durch cellHeight bestimmt.
         *   [Limit: >= 0] [Einheit: Welteinheiten]
         *
         * agentMaxSlope
         *   Maximaler begehbarer Neigungswinkel in Grad. Bei flachem Boden sehr
         *   niedrig setzen (z.B. 5 Grad). Steilere Flächen werden als nicht begehbar
         *   markiert.
         *   [Limit: 0 <= Wert < 90] [Einheit: Grad]
         *
         * ── REGIONS-EINSTELLUNGEN ────────────────────────────────────
         *
         * regionMinSize
         *   Mindestfläche (in Voxeln²) einer isolierten Navmesh-Insel, damit sie
         *   erhalten bleibt. Kleinere Inseln werden als Rauschen verworfen
         *   (Tischflächen, kleine Absätze usw.). Im Labyrinth klein halten, damit
         *   Korridorecken und Sackgassen nicht versehentlich entfernt werden.
         *   Intern wird dieser Wert quadriert: minRegionArea = regionMinSize².
         *   [Limit: >= 0] [Einheit: Voxel]
         *
         * regionMergeSize
         *   Kleine benachbarte Regionen bis zu dieser Größe werden zu größeren
         *   Polygonen zusammengeführt. Größere Werte = weniger, aber größere Polygone
         *   (schnellere Wegfindung, weniger formgenaue Konturen). Für schmale
         *   Korridore moderat halten.
         *   Intern: mergeRegionArea = regionMergeSize².
         *   [Limit: >= 0] [Einheit: Voxel]
         *
         * ── KANTEN-EINSTELLUNGEN ─────────────────────────────────────
         *
         * edgeMaxLen
         *   Maximale Länge einer Navmesh-Polygonkante. Zusätzliche Eckpunkte werden
         *   eingefügt, um Kanten unterhalb dieses Grenzwerts zu halten. Kürzer =
         *   mehr Polygone, aber Kanten folgen Wänden präziser. Wert 0 deaktiviert
         *   diese Funktion vollständig.
         *   Präzision wird durch cellSize beeinflusst.
         *   [Limit: >= 0] [Einheit: Welteinheiten]
         *
         * edgeMaxError  (maxSimplificationError / edgeMaxDeviation)
         *   Maximale Abweichung einer vereinfachten Konturkante vom originalen
         *   Voxel-Umriss in der xz-Ebene. Kleiner = Kanten folgen Wänden präziser,
         *   aber mehr Dreiecke werden erzeugt. Wert 0 wird nicht empfohlen, da er
         *   zu einer Explosion der Polygonanzahl führen kann.
         *   [Limit: >= 0] [Einheit: Welteinheiten]
         *
         * ── POLYGON-EINSTELLUNGEN ────────────────────────────────────
         *
         * vertsPerPoly
         *   Maximale Anzahl von Eckpunkten pro Navmesh-Polygon. Detours harte Grenze
         *   ist DT_VERTS_PER_POLYGON = 6. Höhere Werte erzeugen besser geformte
         *   Polygone bei leicht erhöhten Verarbeitungskosten. Immer 6 verwenden.
         *   [Limit: >= 3, <= 6 für Detour]
         *
         * detailSampleDist  (contourSampleDistance)
         *   Abtastabstand beim Erstellen des Höhen-Detail-Mesh. Bestimmt, wie genau
         *   das Detail-Mesh der Oberfläche der Quellgeometrie folgt. Nicht kritisch
         *   für flache Böden — auf Korridorbreite oder größer setzen. Werte unter
         *   0.9 deaktivieren diese Funktion vollständig.
         *   [Limit: 0 oder >= 0.9] [Einheit: Welteinheiten]
         *
         * detailSampleMaxError  (contourMaxDeviation)
         *   Maximale Abweichung der Detail-Mesh-Oberfläche vom Höhenfeld der
         *   Quellgeometrie. Nicht kritisch für flache Böden. Wert 0 wird nicht
         *   empfohlen, da er zu einer großen Dreieckszahl führen kann.
         *   [Limit: >= 0] [Einheit: Welteinheiten]
         *
         * keepInterResults
         *   Falls true, werden Zwischenstrukturen (Heightfield, Compact Heightfield,
         *   Contour Set) nach der Navmesh-Erstellung im Speicher behalten. Nützlich
         *   für Debug-Rendering. In der Produktion auf false setzen, um Speicher
         *   freizugeben.
         *
         * ── NAVMESH QUERY NODE POOL ──────────────────────────────────
         *
         * m_navQuery->init(m_navMesh, nodePoolSize)
         *   Kein Konfigurationsparameter, aber kritisch für tiefe Labyrinthe.
         *   Der Node Pool ist der interne A*-Suchpuffer. In einem komplexen
         *   Labyrinth muss der Wegfinder viele Polygone erkunden, bevor er das
         *   Ziel erreicht. Ist der Pool zu klein (Standard 2048), gehen A* die
         *   Knoten mitten in der Suche aus und gibt DT_FAILURE zurück — Fehler -3,
         *   obwohl ein gültiger Pfad existiert.
         *
         *   Für Labyrinthe immer 65535 verwenden:
         *     m_navQuery->init(m_navMesh, 65535);
         *
         * ── POINTEXTENDS (LAUFZEIT-EINRASTE-SUCHBOX) ─────────────────
         *
         * pointExtends  (mExtents)
         *   Eine 3D-Halbgrößen-Suchbox, die von findNearestPoly() verwendet wird,
         *   um eine Weltposition (Klickziel, Agentenstart) auf das nächste
         *   Navmesh-Polygon einzurasten. Detour sucht innerhalb von ±extents um
         *   den angegebenen Punkt.
         *
         *     mExtents[0] = X Halbbreite  (links/rechts)
         *     mExtents[1] = Y Halbhöhe    (oben/unten)
         *     mExtents[2] = Z Halbtiefe   (vorne/hinten)
         *
         *   Stell es dir wie einen Magnetradius vor — größer bedeutet weiter
         *   reichende Suche nach einem Polygon. Die GEFAHR im Labyrinth:
         *
         *     Wand
         *     |  Korridor A  |##Wand##|  Korridor B  |
         *           ↑ Klick hier              ↑ Einrasten hier wenn extents zu groß!
         *
         *   Wenn X/Z extents die halbe Korridorbreite überschreiten, kann das
         *   Einrasten DURCH eine Wand hindurch in den benachbarten Korridor
         *   reichen. Detour versucht dann einen Pfad zwischen zwei unverbundenen
         *   Polygonen zu finden und gibt Fehler -3 zurück.
         *
         *   Faustregel:
         *     pointExtends.x < Korridorbreite / 2
         *     pointExtends.z < Korridorbreite / 2
         *     pointExtends.y kann großzügiger sein (Mehretagengebäude, Höhenvariation)
         *
         *   Empfehlung für flaches 2m-Korridorlabyrinth:
         *     pointExtends = Vector3(0.8f, 1.0f, 0.8f)
         *
         * ── EMPFOHLENE EINSTELLUNGEN FÜR EIN FLACHES 2m-LABYRINTH ────
         *
         *   cellSize             = 0.2f    // 10 Voxel pro 2m Korridor
         *   cellHeight           = 0.2f    // flacher Boden
         *   agentHeight          = 1.8f    // Charakterhöhe
         *   agentRadius          = 0.3f    // hinterlässt 1.4m begehbar nach Erosion
         *   agentMaxClimb        = 0.2f    // flacher Boden, klein halten
         *   agentMaxSlope        = 5.0f    // nahezu flach
         *   regionMinSize        = 4
         *   regionMergeSize      = 8
         *   edgeMaxLen           = 4.0f
         *   edgeMaxError         = 1.0f
         *   vertsPerPoly         = 6
         *   detailSampleDist     = 2.0f
         *   detailSampleMaxError = 0.1f
         *   pointExtends         = Vector3(0.8f, 1.0f, 0.8f)
         *   navQuery Node Pool   = 65535
         */
        OgreRecast* createOgreRecast(Ogre::SceneManager* sceneManager, OgreRecastConfigParams params, const Ogre::Vector3& pointExtends = Ogre::Vector3(0.8f, 1.0f, 0.8f));

		/** 
		 * @brief Destroyes the internal navigation content.
		 */
		void destroyContent(void);

		/** 
		 * @brief Adds a static (not movable) obstacle mesh from the given game object id. The obstacle is unwalkable, so the path will be created around this obstacle.
		 * @param[in] id The game object id to get the mesh.
		 */
		void addStaticObstacle(unsigned long id);
		
		/** 
		 * @brief Removes the static (not movable) obstacle mesh from the given game object id. This can be used e.g. to simulate an open or closed gate.
		 * @param[in] id The game object id to get the mesh.
		 */
		void removeStaticObstacle(unsigned long id);

		/** 
		 * @brief Adds a dynamic (movable) obstacle mesh from the given game object id. The obstacle is unwalkable, so the path will be created around this obstacle.
		 *  	  The object can be moved at runtime, so that path will be recalculated.
		 * @param[in] id					The game object id to get the mesh.
		 * @param[in] walkable				Whether this game object mesh is walk able or not
		 * @param[in] externalInputGeom		The external input geom to add, instead creating a new one
		 * @param[in] externalConvexVolume	The external convex volume to add, instead creating a new one
		 */
		void addDynamicObstacle(unsigned long id, bool walkable, InputGeom* externalInputGeom = nullptr, ConvexVolume* externalConvexVolume = nullptr);

		/** 
		 * @brief Removes the dynamic (movable) obstacle mesh from the given game object id.
		 * @param[in] id		The game object id to get the mesh.
		 * @param[in] destroy	If set to true, the internal convex volume and input geom will also be destroyed
		 * @return The InputGeom and ConvexVolue, but only if the parameter destroy is set to false, else the pair with 2x nullptr is delivered
		 */
		std::pair<InputGeom*, ConvexVolume*> removeDynamicObstacle(unsigned long id, bool destroy = true);

		/** 
		 * @brief Activates or deactivates an existing dynamic (movable) obstacle mesh from the given game object id at runtime, without the need of removing it for better performance.
		 * @param[in] id The game object id to get the mesh.
		 */
		void activateDynamicObstacle(unsigned long id, bool activate);

		/** 
		 * @brief Adds terra for input geom mesh from the given game object id.
		 * @param[in] id					The game object id to get the mesh.
		 */
		void addTerra(unsigned long id);

		/** 
		 * @brief Removes terra and input geom mesh.
		 * @param[in] id		The game object id to get the mesh.
		 * @param[in] destroy	If set to true, the internal input geom will also be destroyed
		 * @return				The InputGeom, but only if the parameter destroy is set to false, else the pair with 2x nullptr is delivered
		 */
		InputGeom* removeTerra(unsigned long id, bool destroy = true);

		/** 
		 * @brief Gets whether OgreRecast has been feed with game object meshes yet. This can be used to determine whether to build the navigation mesh or not. 
		 * @return hasNavigationElement true, if there are some navigation elements added by @addStaticObstacle or @addDynamicObstacle
		 */
		bool hasNavigationMeshElements(void) const;

		/** 
		 * @brief Builds the navigation mesh. Its likely that it will not work at the first, as maybe some config params are not tweaked correctly. So an error log is produced and an event fired
		 *		  @eventDataNavigationMeshFeedback so that the developer may react whether the navigation mesh could be created or not.
		 */
		void buildNavigationMesh(void);

		/** 
		 * @brief Debug draws the navigation mesh for better analysis.
		 * @param[in] draw Whether to draw or not
		 */
		void debugDrawNavMesh(bool draw);

		/** 
		 * @brief Updates the Ogre recast navigation data like moving dynamic obstacles etc. and re-adapding navigation mesh.
		 * @param[in] dt The delta time, usually 0.016 = 1 / 60 frames
		 */
		void update(Ogre::Real dt);

	   /** 
		* @brief Tries to find an optimal path. It takes a start point and an end point and, if possible, generates a list of lines in a path.
		* It might fail if the start or end points aren't near any navigation mesh polygons, or if the path is too long, or it can't make a path, or various other reasons.So far I've not had problems though.
		* @param[in] startPosition The start position for the path finding
		* @param[in] endPosition The end position for the path finding
		* @param[in] pathSlot Number identifying the target the path leads to
		* @param[in] targetSlot: The index number for the slot in which the found path is to be stored
		* @param[in] drawPath: Whether to draw the path or not
		* @return	 true, if a path could be found, else false
		*/
		bool findPath(const Ogre::Vector3& startPosition, const Ogre::Vector3& endPosition, int pathSlot, int targetSlot, bool drawPath = false);

		/** 
		* @brief Removes the drawn path.
		*/
		void removeDrawnPath(void);

		/** 
		 * @brief Gets the OgreRecast pointer. From this pointer the internal Recast data structure is callable if the developer wants to more customize navigation.
		 * @return OgreRecast The OgreRecast pointer to get.
		 */
		OgreRecast* getOgreRecast(void) const;

        /** 
         * @brief Gets the OgreDetourTileCache pointer. From this pointer the internal DetourTileCache data structure is callable if the developer wants to more customize navigation.
         * @return OgreDetourTileCache The OgreDetourTileCache pointer to get.
         */
		OgreDetourTileCache* getOgreDetourTileCache(void) const;

        /** 
         * @brief Gets the OgreDetourCrowd pointer. From this pointer the internal DetourCrowd data structure is callable if the developer wants to more customize navigation.
         * @return OgreDetourCrowd The OgreDetourCrowd pointer to get.
         */
		OgreDetourCrowd* getOgreDetourCrowd(void) const;

        /*
        * @brief Checks if the navigation mesh build is in progress.
        * @return true if the build is in progress, false otherwise.
        */
        bool isBuildInProgress(void) const;

        /*
         * @brief Checks if the navigation mesh is valid and can be used for pathfinding.
         * @return true if the navigation mesh is valid, false otherwise.
         */
        bool getHasValidNavMesh(void) const;

        /*
         * @brief Loads the navigation mesh from a file.
         * @return true if the navigation mesh was successfully loaded, false otherwise.
         */
        bool loadNavigationMesh(void);

        /**
         * @brief Handles the event when the geometry of a game object is modified. This can be used to update the navigation mesh if necessary, for example when a dynamic obstacle is moved or changed.
         * @param[in] eventData The event data containing information about the modified geometry.
         */
        void handleGeometryModified(NOWA::EventDataPtr eventData);

        /**
         * @brief Sets whether the navigation mesh must be regenerated.
         * @param[in] mustRegenerate true if the navigation mesh must be regenerated, false otherwise.
         */
        void setMustRegenerate(bool mustRegenerate);
	private:
		OgreRecastModule(const Ogre::String& appStateName);

		~OgreRecastModule();

		void createInputGeom(void);

        Ogre::String getNavMeshFilePath(void) const;

        Ogre::String getNavGeomFilePath(void) const;

        bool saveNavigationMesh(const InputGeom::NavMeshGeomSnapshot& snapshot);
	private:
		Ogre::String appStateName;
		OgreRecast* ogreRecast;
		OgreDetourTileCache* detourTileCache;
		OgreDetourCrowd* detourCrowd;
		std::vector<unsigned long> staticObstacles;
		std::map<unsigned long, std::pair<InputGeom*, ConvexVolume*>> dynamicObstacles;
		std::map<unsigned long, InputGeom*> terraInputGeomCells;
		bool mustRegenerate;
		bool hasValidNavMesh;
		bool debugDraw;
        std::thread navMeshThread;
        std::atomic<bool> buildInProgress;
	};

}; //namespace end

#endif
