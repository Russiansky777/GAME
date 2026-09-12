"""Generate dense, common-world-origin dressing for LOW TIDE's trader hub."""
import bpy, json, math, sys
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]; OUT=ROOT/'SourceAssets'/'HubSlice'; PRE=ROOT/'Artifacts'/'HubSlicePreview'; BLEND=ROOT/'Intermediate'/'HubSlice'
TERRAIN_OUT=ROOT/'Intermediate'/'GeneratedM1'
COMPONENTS=('SM_LT_HubSlice_Apron','SM_LT_HubSlice_WorkCluster','SM_LT_HubSlice_GoodsCluster','SM_LT_HubSlice_MaritimeDetails','SM_LT_HubSlice_GroundDressing','SM_LT_HubSlice_JobBoard')
PAL={'DarkWood':(.16,.065,.025,1),'WarmWood':(.48,.20,.07,1),'TealPaint':(.035,.26,.27,1),'Cream':(.86,.66,.38,1),'Rust':(.62,.13,.045,1),'Brass':(.62,.34,.07,1),'Rope':(.48,.30,.12,1),'GlassGreen':(.035,.30,.19,1),'Shell':(.86,.48,.28,1)}
M={}; O={c:[] for c in COMPONENTS}

def setup():
 bpy.ops.wm.read_factory_settings(use_empty=True); s=bpy.context.scene; s.unit_settings.system='METRIC'; s.unit_settings.scale_length=.01; s.render.engine='BLENDER_EEVEE'; s.render.resolution_x=1100; s.render.resolution_y=700; s.view_settings.look='AgX - Medium High Contrast'; s.world=bpy.data.worlds.new('CoastalWorld'); s.world.use_nodes=True; s.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.06,.13,.16,1); s.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.55
 for n,c in PAL.items():
  q=bpy.data.materials.new(n); q.diffuse_color=c; q.use_nodes=True; bs=q.node_tree.nodes['Principled BSDF']; bs.inputs['Base Color'].default_value=c; bs.inputs['Roughness'].default_value=.72; bs.inputs['Metallic'].default_value=.45 if n=='Brass' else 0; M[n]=q
def tag(o,c,m): o.data.materials.append(M[m]); O[c].append(o); return o
def bevel(o,w=2):
 bpy.context.view_layer.objects.active=o; o.select_set(True); x=o.modifiers.new('soft handmade edges','BEVEL'); x.width=w; x.segments=1; x.limit_method='ANGLE'; bpy.ops.object.modifier_apply(modifier=x.name); o.select_set(False)
def box(n,p,d,m,c,r=0,b=2):
 bpy.ops.mesh.primitive_cube_add(location=p,rotation=(0,0,math.radians(r))); o=bpy.context.object; o.name=n; o.dimensions=d; bpy.ops.object.transform_apply(location=False,rotation=False,scale=True); bevel(o,min(b,min(d)*.25)); return tag(o,c,m)
def cyl(n,p,rad,dep,m,c,verts=12,rot=(0,0,0),b=1):
 bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=rad,depth=dep,location=p,rotation=rot); o=bpy.context.object; o.name=n; bevel(o,b); [setattr(f,'use_smooth',True) for f in o.data.polygons]; return tag(o,c,m)
def torus(n,p,R,r,m,c,rot=(0,0,0),seg=20):
 bpy.ops.mesh.primitive_torus_add(major_radius=R,minor_radius=r,major_segments=seg,minor_segments=5,location=p,rotation=rot); o=bpy.context.object; o.name=n; return tag(o,c,m)
def tube(n,pts,r,m,c,cyclic=False):
 q=bpy.data.curves.new(n,'CURVE'); q.dimensions='3D'; q.resolution_u=1; q.bevel_depth=r; q.bevel_resolution=1; s=q.splines.new('BEZIER'); s.bezier_points.add(len(pts)-1)
 for p,v in zip(s.bezier_points,pts): p.co=v; p.handle_left_type='AUTO'; p.handle_right_type='AUTO'
 s.use_cyclic_u=cyclic; o=bpy.data.objects.new(n,q); bpy.context.collection.objects.link(o); o.data.materials.append(M[m]); O[c].append(o); return o
def plank(n,p,d,c,accent=False,r=-38):
 o=box(n,p,d,'TealPaint' if accent else 'WarmWood',c,r,1.8)
 # inset grain bands and nailheads are real silhouette/surface geometry
 for off in (-d[1]*.22,d[1]*.22): box(n+'Grain',(p[0]+math.sin(math.radians(r))*off,p[1]-math.cos(math.radians(r))*off,p[2]+d[2]/2+.45),(d[0]*.72,1.4,.9),'DarkWood',c,r,.2)
 for sx in (-.38,.38):
  for sy in (-.34,.34):
   lx=sx*d[0]; ly=sy*d[1]; x=p[0]+lx*math.cos(math.radians(r))-ly*math.sin(math.radians(r)); y=p[1]+lx*math.sin(math.radians(r))+ly*math.cos(math.radians(r)); cyl('Nail',(x,y,p[2]+d[2]/2+1),2.1,1.5,'Brass',c,8)
 return o
def _clip_polygon(poly, signed_distance):
 """Clip a 3D polygon against one linear half-plane (positive side retained)."""
 clipped=[]
 for start,end in zip(poly,poly[1:]+poly[:1]):
  ds,de=signed_distance(start),signed_distance(end)
  if ds>=-1e-6: clipped.append(start)
  if (ds>=0) != (de>=0):
   alpha=ds/(ds-de)
   clipped.append(tuple(start[j]+(end[j]-start[j])*alpha for j in range(3)))
 return clipped
def _terrain_top_triangles():
 """Read pre-reflected terrain OBJ exports back into gameplay/Blender coordinates.

 GenerateM1Assets.py owns these canonical route surfaces and must run first in a
 fresh checkout; this keeps the decorative deck exactly tied to gameplay terrain.
 """
 triangles=[]
 for name in ('SM_LT_CoastalTerrainSand.obj','SM_LT_CoastalTerrainStone.obj'):
  if not (TERRAIN_OUT/name).is_file():
   raise RuntimeError(f'Missing {name}; run Scripts/GenerateM1Assets.py before GenerateHubSlice.py')
  vertices=[]
  for line in (TERRAIN_OUT/name).read_text(encoding='utf-8').splitlines():
   fields=line.split()
   if not fields: continue
   if fields[0]=='v': vertices.append((float(fields[1]),-float(fields[2]),float(fields[3])))
   elif fields[0]=='f':
    ids=[int(field.split('/')[0])-1 for field in fields[1:]]
    for index in range(1,len(ids)-1): triangles.append(tuple(vertices[i] for i in (ids[0],ids[index],ids[index+1])))
 return triangles
def _polygon_z(poly,x,y):
 for index in range(1,len(poly)-1):
  a,b,c=poly[0],poly[index],poly[index+1]
  denominator=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
  if abs(denominator)<1e-8: continue
  wa=((b[1]-c[1])*(x-c[0])+(c[0]-b[0])*(y-c[1]))/denominator
  wb=((c[1]-a[1])*(x-c[0])+(a[0]-c[0])*(y-c[1]))/denominator; wc=1-wa-wb
  if min(wa,wb,wc)>=-1e-6: return wa*a[2]+wb*b[2]+wc*c[2]
 return None
def _deck_route_overlay(c):
 """Cover route terrain that crosses the flush deck without changing walk collision."""
 cx,cy=650.0,-550.0; angle=math.radians(-30.0); ca,sa=math.cos(angle),math.sin(angle)
 def local(point):
  dx,dy=point[0]-cx,point[1]-cy
  return ca*dx+sa*dy,-sa*dx+ca*dy
 polygons=[]; source_triangles=0; top_triangles=[]
 for triangle in _terrain_top_triangles():
  a,b,d=triangle
  ux,uy,uz=b[0]-a[0],b[1]-a[1],b[2]-a[2]; vx,vy,vz=d[0]-a[0],d[1]-a[1],d[2]-a[2]
  nx,ny,nz=uy*vz-uz*vy,uz*vx-ux*vz,ux*vy-uy*vx
  # Keep traversable top faces, excluding route skirts and cliff/down faces.
  if abs(nz)/math.sqrt(nx*nx+ny*ny+nz*nz)<.5: continue
  # OBJ faces are wound for their pre-reflected coordinates; restore upward winding too.
  poly=list(triangle if nz>0 else reversed(triangle))
  top_triangles.append(poly)
  for plane in (
   lambda p: local(p)[0]+1040.0, lambda p: 1040.0-local(p)[0],
   lambda p: local(p)[1]+750.0, lambda p: 750.0-local(p)[1],
   lambda p: p[2]-126.0):
   poly=_clip_polygon(poly,plane)
   if len(poly)<3: break
  if len(poly)>=3:
   source_triangles+=1
   polygons.append([(x,y,z+2.0) for x,y,z in poly])
 if not polygons: raise RuntimeError('Deck overlay found no elevated terrain within apron')
 vertices=[]; faces=[]
 for poly in polygons:
  base=len(vertices); vertices.extend(poly)
  for index in range(1,len(poly)-1): faces.append((base,base+index,base+index+1))
 mesh=bpy.data.meshes.new('WorkingDeckRouteOverlayMesh'); mesh.from_pydata(vertices,[],faces); mesh.update()
 overlay=bpy.data.objects.new('WorkingDeckRouteOverlay',mesh); bpy.context.collection.objects.link(overlay); tag(overlay,c,'WarmWood')
 # Reintroduce narrow plank seams across the conforming skin without exposing terrain.
 seam_polygons=[]
 for poly in polygons:
  unraised=[(x,y,z-2.0) for x,y,z in poly]
  for seam_x in range(-960,1000,80):
   seam=_clip_polygon(unraised,lambda p,s=seam_x: local(p)[0]-(s-1.2))
   seam=_clip_polygon(seam,lambda p,s=seam_x: (s+1.2)-local(p)[0]) if seam else []
   if len(seam)>=3: seam_polygons.append([(x,y,z+2.5) for x,y,z in seam])
 if seam_polygons:
  seam_vertices=[]; seam_faces=[]
  for poly in seam_polygons:
   base=len(seam_vertices); seam_vertices.extend(poly)
   for index in range(1,len(poly)-1): seam_faces.append((base,base+index,base+index+1))
  seam_mesh=bpy.data.meshes.new('WorkingDeckRouteSeamsMesh'); seam_mesh.from_pydata(seam_vertices,[],seam_faces); seam_mesh.update()
  seam_object=bpy.data.objects.new('WorkingDeckRouteSeams',seam_mesh); bpy.context.collection.objects.link(seam_object); tag(seam_object,c,'DarkWood')
 # Static proof: every sampled elevated terrain point in the deck has skin at terrain +2 cm.
 samples=0; max_error=0.0
 for local_x in range(-1020,1021,40):
  for local_y in range(-740,741,40):
   x=cx+local_x*ca-local_y*sa; y=cy+local_x*sa+local_y*ca
   terrain=[z for poly in top_triangles if (z:=_polygon_z(poly,x,y)) is not None and z>126.0001]
   if not terrain: continue
   skin=[z for poly in polygons if (z:=_polygon_z(poly,x,y)) is not None]
   if not skin: raise RuntimeError(f'Deck overlay misses elevated terrain at {x:.2f},{y:.2f}')
   error=abs(max(skin)-(max(terrain)+2.0)); max_error=max(max_error,error); samples+=1
 if samples==0 or max_error>.01: raise RuntimeError(f'Deck overlay surface validation failed: {samples} samples, error {max_error:.4f}')
 print(f'[GenerateHubSlice] deck overlay: {source_triangles} clipped terrain triangles, {len(faces)} skin + {sum(len(p)-2 for p in seam_polygons)} seam triangles, Z {min(v[2] for v in vertices):.3f}..{max(v[2] for v in vertices):.3f}; {samples} elevated samples, max error {max_error:.4f} cm')
def crate(p,s,c,accent=False):
 x,y,z=p; sx,sy,sz=s
 for zz in (z+sz*.18,z+sz*.48,z+sz*.78): box('CrateSlat',(x,y,zz),(sx,sy,sz*.22),'Cream',c,-8,1.5)
 for yy in (-sy*.43,sy*.43): box('CrateBand',(x,y+yy,z+sz*.5),(sx+5,8,sz),'TealPaint' if accent else 'DarkWood',c,-8,1.5)
 for xx in (-sx*.43,sx*.43): box('CrateBand',(x+xx,y,z+sz*.5),(8,sy+5,sz),'DarkWood',c,-8,1.5)
def barrel(p,rad,h,c):
 x,y,z=p
 cyl('Barrel',(x,y,z+h/2),rad,h,'WarmWood',c,14,b=2)
 for zz in (z+12,z+h/2,z+h-12): torus('BarrelHoop',(x,y,zz),rad+1,3,'Brass',c,seg=18)
 for a in range(0,360,45): box('StaveGrain',(x+math.cos(math.radians(a))*rad*.72,y+math.sin(math.radians(a))*rad*.72,z+h+.8),(rad*.45,1,.8),'DarkWood',c,a,.1)
def apron():
 c=COMPONENTS[0]
 # One continuous flush working deck spans the camera foreground, shop apron,
 # and first route meters. A single field avoids hidden overlap and z-fighting.
 a=math.radians(-30); cx,cy=650,-550
 for i in range(26):
  lx=-1000+i*80; ly=(i%4-1.5)*2.2
  x=cx+lx*math.cos(a)-ly*math.sin(a); y=cy+lx*math.sin(a)+ly*math.cos(a)
  plank('WorkingDeckPlank',(x,y,121.5),(76,1500,9),c,i in (2,10,18,24),r=-30)
 _deck_route_overlay(c)
def work_cluster():
 c=COMPONENTS[1]; x,y=250,-900
 box('Worktop',(x,y,221),(210,115,18),'WarmWood',c,-8,5)
 for j in (-35,0,35): box('WorktopGrain',(x+j,y-2,231),(2,82,1.4),'DarkWood' if j else 'Cream',c,-8,.2)
 for dx in (-82,82):
  for dy in (-38,38): cyl('WorktopNail',(x+dx,y+dy,232),2.5,2,'Brass',c,8,b=.4)
 for dx in (-82,82):
  for dy in (-38,38): box('SplayedLeg',(x+dx,y+dy,172),(18,18,100),'DarkWood',c,-8+(-3 if dx<0 else 3),3)
 # map, route marks, mug, hand tools, bucket, stacked rope and two crates
 box('Map',(x-5,y,233),(150,92,2),'Cream',c,-8,1)
 tube('MapRoute',[(x-55,y-20,235),(x-15,y+4,236),(x+25,y-12,235),(x+55,y+22,236)],2,'TealPaint',c)
 cyl('Mug',(x-82,y+38,239),6,14,'TealPaint',c,14,b=1); torus('MugHandle',(x-90,y+38,239),4.5,1.5,'Brass',c,rot=(math.pi/2,0,0),seg=14)
 for j in range(3): box('Tool',(x+35+j*25,y+32-j*10,238),(55,7,6),'Brass' if j==1 else 'DarkWood',c,-25+j*18,2)
 cyl('Bucket',(75,-965,146),18,40,'TealPaint',c,14,b=1.5); tube('BucketHandle',[(60,-965,166),(75,-949,181),(90,-965,166)],1.8,'Brass',c)
 crate((88,-1080,125),(98,78,82),c,True); crate((-15,-1060,125),(82,68,64),c)
 for j,R in enumerate((42,34,26)): torus('RopeCoil',(35,-850,131+j*5),R,5,'Rope',c,seg=22)
def goods_cluster():
 c=COMPONENTS[2]; ox,oy=1900,750
 crate((ox,oy,125),(135,100,105),c,True); crate((ox+115,oy-60,125),(100,88,78),c); barrel((ox+230,oy-130,125),48,118,c)
 # three woven baskets with distinct sorted contents
 for j,(x,y) in enumerate(((ox-140,oy-160),(ox-20,oy-260),(ox+160,oy-280))):
  cyl('Basket',(x,y,159),55,68,'Rope',c,14,b=2); torus('BasketRim',(x,y,194),56,5,'DarkWood',c,seg=20)
  for k in range(7):
   a=2*math.pi*k/7; mat=('Shell','GlassGreen','Rust')[j]; cyl('SortedGood',(x+math.cos(a)*27,y+math.sin(a)*27,201+(k%2)*7),10 if j!=0 else 13,18,mat,c,10,b=1)
 # bottle shelf and rolled canvas bundle
 box('GoodsShelf',(ox+360,oy-30,205),(190,48,15),'DarkWood',c,-16,3)
 shelf_a=math.radians(-16)
 for lx in (-76,76):
  for ly in (-15,15):
   sx=ox+360+lx*math.cos(shelf_a)-ly*math.sin(shelf_a)
   sy=oy-30+lx*math.sin(shelf_a)+ly*math.cos(shelf_a)
   box('GoodsShelfLeg',(sx,sy,161),(13,13,72),'WarmWood',c,-16,2)
 box('GoodsShelfBrace',(ox+360,oy-30,151),(160,10,12),'TealPaint',c,-16,2)
 for j in range(5):
  bx=ox+300+j*30; cyl('Bottle',(bx,oy-30,231),11,45,'GlassGreen',c,12,b=2); cyl('BottleNeck',(bx,oy-30,259),5,18,'Brass',c,10,b=1)
 for j in range(4): cyl('CanvasRoll',(ox-230,oy-10+j*35,154),18,155,'Rust' if j%2 else 'Cream',c,12,rot=(0,math.pi/2,0),b=2)
def maritime():
 c=COMPONENTS[3]
 # freestanding drying/net rack well south of route
 for x in (930,1370): box('NetPost',(x,-2700,260),(28,28,270),'DarkWood',c,-5,4)
 box('NetTop',(1150,-2700,385),(470,24,24),'WarmWood',c,-5,4)
 for i in range(8):
  x=955+i*56; tube('NetDrop',[(x,-2694,370),(x+8,-2710,295),(x-4,-2698,220)],2,'Rope',c)
 for j in range(4): tube('NetCross',[(950,-2698,235+j*34),(1150,-2720-(j%2)*8,225+j*35),(1370,-2698,235+j*34)],2,'Rope',c)
 for x in (1000,1260): torus('NetFloat',(x,-2685,315),15,5,'Cream',c,rot=(math.pi/2,0,0),seg=14)
 # buoy pole, hook and rope-bound driftwood
 box('BuoyPole',(-420,-1510,250),(24,24,250),'TealPaint',c,8,4); tube('Hook', [(-420,-1510,370),(-430,-1490,335),(-414,-1480,310)],5,'Brass',c)
 for j in range(3): box('Driftwood',(-500+j*55,-1670-j*22,145),(170,25,28),'WarmWood',c,24-j*17,5)
 for z in (146,165): torus('Binding',(-450,-1650,z),36,4,'Rope',c,rot=(math.pi/2,0,0),seg=16)
def ground_dressing():
 c=COMPONENTS[4]
 # low edge accents; shells are deliberately smooth and asymmetrical
 stones=[(-720,-1320,18,34,24),(230,-2280,20,42,28),(2070,-2710,24,55,36),(2350,-2470,18,38,25),(-520,-2100,15,32,22)]
 for i,(x,y,z,sx,sy) in enumerate(stones): box('EdgeStone',(x,y,125+z/2),(sx,sy,z),'Shell' if i==1 else 'DarkWood',c,i*19,5)
 for i,(x,y) in enumerate(((-650,-1450),(-590,-1490),(2210,-2550),(2290,-2600),(260,-2140),(320,-2180))):
  cyl('Shell',(x,y,132),18+(i%2)*5,10,'Shell',c,14,rot=(0,math.radians(68),math.radians(i*31)),b=2)
  torus('ShellLip',(x,y,136),11+(i%2)*3,2,'Cream',c,rot=(math.pi/2,0,0),seg=14)
 for x,y,r in ((-760,-1780,26),(2180,-2320,32),(350,-2350,22)):
  for k in range(9):
   a=2*math.pi*k/9; cyl('Pebble',(x+math.cos(a)*r,y+math.sin(a)*r,128),3+k%3,5,'Cream' if k%3 else 'Shell',c,8,b=.5)
def job_board():
 c=COMPONENTS[5]
 # Local origin at ground center. Front faces -X; Unreal places at (1450,170,125), yaw 7.
 for y in (-132,132):
  box('BoardPost',(8,y,142),(30,30,284),'DarkWood',c,0,5); box('PostFoot',(8,y,12),(52,52,24),'Brass',c,0,5)
  for z in (105,165,225): box('PostGrain',(-8,y,z),(2,10,34),'WarmWood',c,0,.3)
  for z in (82,252): cyl('PostNail',(-9,y,z),3,2,'Brass',c,9,rot=(0,math.pi/2,0),b=.5)
 box('BoardBack',(12,0,172),(22,250,205),'WarmWood',c,0,5)
 for y in (-110,-55,0,55,110): box('BackPlank',(-1,y,172),(9,48,192),'Cream' if y else 'TealPaint',c,0,2)
 box('TopFrame',(-8,0,287),(34,292,30),'DarkWood',c,0,5); box('BottomFrame',(-8,0,66),(34,292,28),'DarkWood',c,0,5)
 for y in (-140,140): box('SideFrame',(-8,y,176),(34,28,220),'DarkWood',c,0,5)
 # Curved, layered rain shade made of radial slats rather than one flat slab.
 for i in range(9):
  y=-144+i*36; z=311-abs(i-4)*2.6; box('ShadeSlat',(-26,y,z),(112,33,13),'Rust' if i in (0,8) else 'TealPaint',c,0,3)
 tube('ShadeFrontEdge',[(-82,-154,306),(-88,0,324),(-82,154,306)],6,'Brass',c)
 # Header backing is intentionally blank: root overlays crisp managed JOBS text.
 box('JobsHeader',(-28,0,269),(18,164,38),'TealPaint',c,0,6)
 box('HeaderInset',(-38,0,269),(4,140,24),'Cream',c,0,2)
 # Thick layered paper notes with folded corners, colored job tabs and brass pins.
 notes=[(-22,-73,218,76,66,'Cream',-4),(-25,18,215,92,76,'Cream',3),(-23,86,229,54,54,'Rust',-2),(-24,-88,135,58,63,'Shell',3),(-25,55,133,103,69,'Cream',-3)]
 for i,(x,y,z,w,h,mat,ang) in enumerate(notes):
  box('JobNote',(x,y,z),(4,w,h),mat,c,ang,1.2); cyl('BoardPin',(x-4,y,z+h*.34),5,7,'Brass',c,10,rot=(0,math.pi/2,0),b=.7)
  box('JobRule',(x-4,y,z),(2,w*.56,3),'DarkWood',c,ang,.2)
 # Central tide-route map: shoreline marks, route cord and destination tokens.
 box('TideMap',(-30,-5,149),(4,100,83),'Cream',c,-2,1.5)
 tube('TideRoute',[(-34,-37,128),(-35,-12,150),(-35,18,139),(-35,39,171)],2.4,'TealPaint',c)
 for y,z in ((-37,128),(-12,150),(18,139),(39,171)): cyl('MapMarker',(-38,y,z),4,5,'Rust',c,9,rot=(0,math.pi/2,0),b=.5)
 # Cross-board string with hanging request tags gives depth and a working-board read.
 tube('JobString',[(-35,-118,188),(-44,-52,173),(-42,12,184),(-43,74,170),(-35,118,184)],2.5,'Rope',c)
 for y,z in ((-52,173),(12,184),(74,170)):
  torus('StringKnot',(-45,y,z),6,2,'Brass',c,rot=(0,math.pi/2,0),seg=12); box('HangingTag',(-47,y,z-18),(4,28,25),'Rust',c,(y/8),1)
 # Side lantern with full cage and handle.
 cyl('BoardLanternBase',(-12,-178,115),20,11,'Brass',c,12,b=2); cyl('BoardLanternGlow',(-12,-178,143),15,46,'Cream',c,12,b=3); cyl('BoardLanternCap',(-12,-178,172),20,12,'Brass',c,12,b=2)
 for a in range(0,360,90): box('BoardLanternCage',(-12+math.cos(math.radians(a))*17,-178+math.sin(math.radians(a))*17,144),(4,4,55),'Brass',c,0,1)
 tube('BoardLanternHandle',[(-12,-196,174),(-12,-190,201),(-12,-166,201),(-12,-160,174)],3,'Brass',c)
def convert_join():
 joined=[]
 for c in COMPONENTS:
  for o in list(O[c]):
   if o.type!='MESH': bpy.context.view_layer.objects.active=o; o.select_set(True); bpy.ops.object.convert(target='MESH'); o.select_set(False)
  bpy.ops.object.select_all(action='DESELECT'); [o.select_set(True) for o in O[c]]; bpy.context.view_layer.objects.active=O[c][0]; bpy.ops.object.join(); o=bpy.context.object; o.name=c; bpy.ops.object.material_slot_remove_unused(); bpy.ops.object.transform_apply(location=True,rotation=True,scale=True); O[c]=[o]; joined.append(o)
 return joined
def stats(o):
 me=o.evaluated_get(bpy.context.evaluated_depsgraph_get()).to_mesh(); me.calc_loop_triangles(); tri=len(me.loop_triangles); verts=len(me.vertices); p=[o.matrix_world@Vector(c) for c in o.bound_box]; return verts,tri,[min(q[i] for q in p) for i in range(3)],[max(q[i] for q in p) for i in range(3)]
def export(o):
 d=o.copy(); d.data=o.data.copy(); bpy.context.collection.objects.link(d)
 for v in d.data.vertices:v.co.y*=-1
 for p in d.data.polygons:p.flip()
 bpy.ops.object.select_all(action='DESELECT'); d.select_set(True); bpy.context.view_layer.objects.active=d; bpy.ops.wm.obj_export(filepath=str(OUT/(o.name+'.obj')),export_selected_objects=True,export_materials=True,export_triangulated_mesh=True,forward_axis='Y',up_axis='Z'); bpy.data.objects.remove(d,do_unlink=True)
def preview():
 bpy.ops.mesh.primitive_plane_add(size=6500,location=(500,-900,119)); g=bpy.context.object; g.data.materials.append(M['Cream'])
 bpy.ops.object.light_add(type='SUN',location=(-2000,-1000,3500)); sun=bpy.context.object; sun.data.energy=3; sun.data.color=(1,.72,.42); sun.rotation_euler=(math.radians(28),math.radians(-20),math.radians(-35))
 bpy.ops.object.light_add(type='AREA',location=(700,-1100,1800)); bpy.context.object.data.energy=250000; bpy.context.object.data.size=1800
 # Display the local-origin board at its integration anchor for review only.
 board=O['SM_LT_HubSlice_JobBoard'][0]; board_preview=board.copy(); board_preview.data=board.data; bpy.context.collection.objects.link(board_preview); board.hide_render=True; board_preview.hide_render=False; board_preview.location=(1450,170,125); board_preview.rotation_euler[2]=math.radians(7)
 bpy.ops.object.camera_add(location=(-2100,-3100,1000)); cam=bpy.context.object; cam.data.lens=45; cam.data.clip_end=20000; cam.rotation_euler=((Vector((750,-900,220))-cam.location).to_track_quat('-Z','Y').to_euler()); bpy.context.scene.camera=cam; bpy.context.scene.render.filepath=str(PRE/'HubSlice_Overview.png'); bpy.ops.render.render(write_still=True)
def main():
 OUT.mkdir(parents=True,exist_ok=True); PRE.mkdir(parents=True,exist_ok=True); BLEND.mkdir(parents=True,exist_ok=True); setup(); apron(); work_cluster(); goods_cluster(); maritime(); ground_dressing(); job_board(); joined=convert_join(); rows=[]; total=0
 apron_only='--apron-only' in sys.argv
 work_only='--work-only' in sys.argv
 for o in joined:
  v,t,mn,mx=stats(o); total+=t
  if (not apron_only and not work_only) or (apron_only and o.name==COMPONENTS[0]) or (work_only and o.name==COMPONENTS[1]): export(o)
  is_board=o.name=='SM_LT_HubSlice_JobBoard'; rows.append({'name':o.name,'obj':o.name+'.obj','intendedUnrealAsset':'/Game/Generated/HubSlice/'+o.name,'collision':False,'worldOrigin':not is_board,'localOrigin':is_board,'suggestedPlacement':{'worldLocationCm':[1450,170,125],'yawDegrees':7,'front':'local -X'} if is_board else None,'units':'cm','groundTopZ':126.0,'role':{'SM_LT_HubSlice_Apron':'deck','SM_LT_HubSlice_WorkCluster':'working-clutter','SM_LT_HubSlice_GoodsCluster':'sorted-goods','SM_LT_HubSlice_MaritimeDetails':'nautical-identity','SM_LT_HubSlice_GroundDressing':'edge-dressing','SM_LT_HubSlice_JobBoard':'interactive hero jobs board'}[o.name],'castShadows':o.name!='SM_LT_HubSlice_GroundDressing','materials':[s.material.name for s in o.material_slots if s.material],'vertices':v,'triangles':t,'boundsCm':{'min':[round(x,2) for x in mn],'max':[round(x,2) for x in mx]}})
 manifest={'assetFamily':'LOW TIDE Hub Slice Dressing','generator':'Scripts/GenerateHubSlice.py','authorship':'Project-authored procedural geometry; no external assets or textures.','units':'cm','worldOrigin':True,'groundTopZ':126.0,'objImport':{'preReflectedY':True,'importUniformScale':1,'combineMeshes':True,'generateCollision':False,'importMaterials':False},'palette':{n:list(c) for n,c in PAL.items()},'meshes':rows,'totalTriangles':total,'triangleBudget':50000,'withinBudget':total<=50000,'routeClearance':'Decorative meshes have no collision. CoastalScene.BuildHubSlice applies runtime work/goods group offsets; review current composition separately from source coordinates.'}
 (OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
 if not apron_only and not work_only: bpy.ops.wm.save_as_mainfile(filepath=str(BLEND/'LT_HubSlice_Source.blend')); preview()
 if total>50000: raise RuntimeError(f'triangle budget exceeded: {total}')
 print(f'[GenerateHubSlice] {len(joined)} components, {total} triangles')
if __name__=='__main__': main()
