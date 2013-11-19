#include "GLDrawer.h"


GLDrawer::GLDrawer(RichParameterSet* _para)
{
	para = _para;
  generateRandomColorList();
}


GLDrawer::~GLDrawer(void)
{
}

void GLDrawer::updateDrawer(vector<int>& pickList)
{
	bCullFace = para->getBool("Need Cull Points");
	bUseIndividualColor = para->getBool("Show Individual Color");
	useNormalColor = para->getBool("Use Color From Normal");
  useDifferBranchColor = para->getBool("Use Differ Branch Color");
  bShowSlice = global_paraMgr.poisson.getBool("Show Slices Mode");

	original_draw_width = para->getDouble("Original Draw Width");
	sample_draw_width = para->getDouble("Sample Draw Width");

	original_color = para->getColor("Original Point Color");
	sample_color = para->getColor("Sample Point Color");
	normal_width = para->getDouble("Normal Line Width");
	normal_length  = para->getDouble("Normal Line Length");
	sample_dot_size = para->getDouble("Sample Dot Size");
  iso_dot_size = para->getDouble("ISO Dot Size");
	original_dot_size = para->getDouble("Original Dot Size");
	normal_color = para->getColor("Normal Line Color");
	feature_color = para->getColor("Feature Color");
	pick_color = para->getColor("Pick Point Color");

	skel_bone_color = para->getColor("Skeleton Bone Color");
	skel_node_color = para->getColor("Skeleton Node Color");
	skel_branch_color = para->getColor("Skeleton Branch Color");
	skel_bone_width = para->getDouble("Skeleton Bone Width") / 10000.;
	skel_node_size = para->getDouble("Skeleton Node Size") / 10000.;

  slice_color_scale = global_paraMgr.glarea.getDouble("Slice Color Scale");
  iso_step_size = global_paraMgr.glarea.getDouble("ISO Interval Size");
  bUseIsoInteral = global_paraMgr.glarea.getBool("Use ISO Interval");
  bUseConfidenceColor = global_paraMgr.drawer.getBool("Show Confidence Color");
  cofidence_color_scale = global_paraMgr.glarea.getDouble("Confidence Color Scale");
  iso_value_shift = global_paraMgr.glarea.getDouble("ISO Value Shift");
  bShowGridCenters = global_paraMgr.glarea.getBool("Show NBV Grids");
  bShowHitGrids = global_paraMgr.glarea.getBool("Show NBV Ray Hit");
  
  bUseConfidenceSeperation = global_paraMgr.nbv.getBool("Use Confidence Seperation");
  confidence_seperation_value = global_paraMgr.nbv.getDouble("Confidence Seperation Value");

  
	if (!pickList.empty())
	{
		curr_pick_indx = pickList[0];
	}

}



void GLDrawer::draw(DrawType type, CMesh* _mesh)
{
	if (!_mesh)
	{
		return;
	}

	bool doPick = para->getBool("Doing Pick");

	int qcnt = 0;
	CMesh::VertexIterator vi;
	for(vi = _mesh->vert.begin(); vi != _mesh->vert.end(); ++vi) 
	{
		if (doPick)
		{
			glLoadName(qcnt);
		}

		if (vi->is_skel_ignore)
		{
			continue;
		}

		Point3f& p = vi->P();      
		Point3f& normal = vi->N();

    if(!(bCullFace && !vi->is_original) || isCanSee(p, normal))		
    {
			switch(type)
			{
			case DOT:
				drawDot(*vi);
				break;
			case CIRCLE:
				drawCircle(*vi);
				break;
			case QUADE:
				drawQuade(*vi);
				break;
			case NORMAL:
				drawNormal(*vi);
				break;
			case SPHERE:
				drawSphere(*vi);
				break;
			default:
				break;
			}
		}

		if (doPick) 
		{
			qcnt++;
		}
	}

	para->setValue("Doing Pick", BoolValue(false));
}

bool GLDrawer::isCanSee(const Point3f& pos, const Point3f& normal)
{
	return  ( (view_point - pos) * normal >= 0 );
}

GLColor GLDrawer::isoValue2color(double iso_value, 
                                 double scale_threshold,
                                 double shift,
                                 bool need_negative)
{
  iso_value += shift;
  //if (!bShowSlice)
  //{
  //  iso_value += shift;
  //}
  
  if (scale_threshold <= 0)
  {
    scale_threshold = 1;
  }

  iso_value = (std::min)((std::max)(iso_value, -scale_threshold), scale_threshold);
  
  bool is_inside = true;
  if (iso_value < 0)
  {
    if (!need_negative)
    {
      iso_value = 0;
    }
    else
    {
      iso_value /= -scale_threshold;
    }
  }
  else
  {
    iso_value /= scale_threshold;
    is_inside = false;
  }

  vector<Point3f> base_colors(5);
  double step_size = 1.0 / (base_colors.size()-1);
  int base_id = iso_value / step_size;
  if (base_id == base_colors.size()-1)
  {
    base_id = base_colors.size()-2;
  }
  Point3f mixed_color;

  if (is_inside && need_negative)
  {
    base_colors[4] = Point3f(0.0, 0.0, 1.0);
    base_colors[3] = Point3f(0.0, 0.7, 1.0);
    base_colors[2] = Point3f(0.0, 1.0, 1.0);
    base_colors[1] = Point3f(0.0, 1.0, 0.7);
    base_colors[0] = Point3f(0.0, 1.0, 0.0);
  }
  else
  {
    base_colors[4] = Point3f(1.0, 0.0, 0.0);
    base_colors[3] = Point3f(1.0, 0.7, 0.0);
    base_colors[2] = Point3f(1.0, 1.0, 0.0);
    base_colors[1] = Point3f(0.7, 1.0, 0.0);
    base_colors[0] = Point3f(0.0, 1.0, 0.0);
  }

  mixed_color = base_colors[base_id] * (base_id * step_size + step_size - iso_value)
                + base_colors[base_id + 1] * (iso_value - base_id * step_size);

  return GLColor(mixed_color.X() / float(step_size), 
                 mixed_color.Y() / float(step_size), 
                 mixed_color.Z() / float(step_size));
}

GLColor GLDrawer::getColorByType(const CVertex& v)
{
  if (v.is_model)
  {
    return cBlack;
  }

  if (v.is_scanned && v.is_scanned_visible)
  {
    return cRed;
  }

	if (v.is_original)
	{
    if (bUseConfidenceColor && v.eigen_confidence >= -0.5)
    {
      return isoValue2color(v.eigen_confidence, cofidence_color_scale, iso_value_shift, true);
    }
		return original_color;
	}

	if (useNormalColor)
	{
		if (RGB_normals.empty())
		{
			//return GLColor( v.cN()[0] , v.cN()[1] , v.cN()[2] );
			return GLColor( (v.cN()[0] + 1) / 2.0 , (v.cN()[1] + 1) / 2.0 , (v.cN()[2]+ 1) / 2.0 );
		}
		else
		{
			Point3f normal = Point3f(v.cN());
			vector<double> color(3, 0.0);
			for(int i = 0; i < 3; i++)
			{
				color[i] = normal * RGB_normals[i];
				if(color[i] < 0)
				{
					color[i] = 0.;		
				}
				//color[i] = (normal * RGB_normals[i] + 1.5) / 2.0;
			}
			return GLColor( color[0], color[1], color[2]);
		}

	}

  if (bShowHitGrids && v.is_iso)
  {
    return cBlue;
  }

  if (bUseConfidenceColor && !v.is_iso && v.eigen_confidence >= -0.5)
  {
    return isoValue2color(v.eigen_confidence, cofidence_color_scale, iso_value_shift, true);
  }

  if (bUseConfidenceColor && v.is_iso)
  {
    return isoValue2color(v.eigen_confidence, cofidence_color_scale, iso_value_shift, true);
  }
	if (bUseIndividualColor && v.is_iso)
	{
    if (v.is_iso && v.is_hole)
    {
      return feature_color;
    }
		//Color4b c = v.C();
  //  return GLColor((255 - slice_color_scale * c.X())/255., c.Y()/255., c.Z()/255., 1.);
    return isoValue2color(v.eigen_confidence, slice_color_scale, iso_value_shift, true);
	}
  else if (v.is_iso)
  {
    if (v.is_boundary)        return cRed;
    if (v.is_view_candidates) return cOrange;
    if (v.is_hole)            return cBlue;
    
    return cGreen;
  }
	else if (v.is_fixed_sample)
	{
		return feature_color;
	}

  if (v.is_grid_center)
  {
    if (v.is_ray_hit) 
    {
      return isoValue2color(v.eigen_confidence, slice_color_scale, iso_value_shift, true);
    }

    if (v.is_ray_stop) return cOrange;
    
    return cBlack;
  }
  
	return sample_color;
}

void GLDrawer::drawDot(const CVertex& v)
{

  //if (bShowGridCenters && v.is_grid_center && !v.is_ray_stop)
  //{
  //  return;
  //}

  if (bUseConfidenceSeperation && v.is_grid_center)
  {
    if (v.eigen_confidence < confidence_seperation_value)
    {
      return;
    }
  }

	int size;
  if (v.is_model)
  {
    size = original_dot_size;
  }
  else if (v.is_original)
	{
		size = original_dot_size;
	}
  else if(v.is_iso)
  {
    size = iso_dot_size;
  }
	else
	{
		size = sample_dot_size;
	}

	glPointSize(size);
	glBegin(GL_POINTS);

	GLColor color = getColorByType(v);
	glColor4f(color.r, color.g, color.b, 1);
	Point3f p = v.P();

  //p.X() += 2 * int(v.eigen_confidence / iso_step_size);
  if (bUseIsoInteral)
  {
    p.X() += 2 * int(v.eigen_confidence / iso_step_size);
  }

	glVertex3f(p[0], p[1], p[2]);
	glEnd(); 
}

void GLDrawer::drawSphere(const CVertex& v)
{
	double radius;
	if (v.is_original)
	{
		radius = original_draw_width;
	}
	else
	{
		radius = sample_draw_width;
	}
	
	GLColor color = getColorByType(v);
	glColor4f(color.r, color.g, color.b, 1);

	Point3f p = v.P();

	glDrawSphere(p, color, radius, 20);
}

void GLDrawer::drawCircle(const CVertex& v)
{
	double radius = sample_draw_width;
	GLColor color = getColorByType(v);

	Point3f p = v.P(); 
	Point3f normal = v.cN();
	Point3f V1 = v.eigen_vector0;
	Point3f V0 = v.eigen_vector1;

	Point3f temp_V = V1 ^ normal;
	float temp = temp_V * V0;

	glNormal3f(normal[0], normal[1], normal[2]);
	glBegin(GL_POLYGON);
	glColor4f(color.r, color.g, color.b, 1);

	int nn = 30;
	for (int i = 0; i < nn; i++)
	{
		float tt1, tt2;
		float rad = 2*3.1415926/nn*i;
		float a, b; 
		a = radius;
		b = radius;

		tt2 = b * sin(rad);
		tt1 = a * cos(rad);

		Point3f vv;

		if (temp < 0)
		{
			vv = V1 * tt1 + V0 * tt2;
		}
		else
		{
			vv = V1 * tt1 - V0 * tt2;
		}
		glVertex3d(p[0]+vv[0], p[1]+vv[1], p[2]+vv[2]);
	}

	glEnd();
}

void GLDrawer::drawQuade(const CVertex& v)
{
	double h = sample_draw_width;
	
	GLColor color = getColorByType(v);
	glColor4f(color.r, color.g, color.b, 1);

	Point3f p = v.P(); 
	Point3f normal = v.cN();
	Point3f V1 = v.eigen_vector0;
	Point3f V0 = v.eigen_vector1;

	//const double *m = v.m;
	glBegin(GL_QUADS);
	
	Point3f temp_V = V1 ^ normal;
	float temp = temp_V * V0;

	if (temp > 0)
	{
		Point3f p0 = p + V0 * h;
		Point3f p1 = p + V1 * h;
		Point3f p2 = p + (-V0) * h;
		Point3f p3 = p + (-V1) * h;

		glNormal3d(normal.X(), normal.Y(), normal.Z());
		glVertex3d(p0.X(), p0.Y(), p0.Z());
		glVertex3d(p1.X(), p1.Y(), p1.Z());
		glVertex3d(p2.X(), p2.Y(), p2.Z());
		glVertex3d(p3.X(), p3.Y(), p3.Z());
	}
	else
	{
		//cout << "negtive" << endl;
		Point3f p0 = p + (-V1) * h;
		Point3f p1 = p + (-V0) * h;
		Point3f p2 = p + V1 * h;
		Point3f p3 = p + V0 * h;

		glNormal3d(normal.X(), normal.Y(), normal.Z());
		glVertex3d(p0.X(), p0.Y(), p0.Z());
		glVertex3d(p1.X(), p1.Y(), p1.Z());
		glVertex3d(p2.X(), p2.Y(), p2.Z());
		glVertex3d(p3.X(), p3.Y(), p3.Z());
	}

	glEnd();  
}

void GLDrawer::drawNormal(const CVertex& v)
{
  if (bUseConfidenceSeperation && v.is_grid_center)
  {
    if (v.eigen_confidence < confidence_seperation_value)
    {
      return;
    }
  }

	double width = normal_width;
	double length = normal_length;
	QColor qcolor = normal_color;

	//glDisable(GL_LIGHTING);

	glLineWidth(width); 
	GLColor color(qcolor);
	glColor4f(color.r, color.g, color.b, 1);  

	Point3f p = v.P(); 
	Point3f m = v.cN();

	glBegin(GL_LINES);	
	glVertex3d(p[0], p[1], p[2]);
	glVertex3f(p[0] + m[0]*length, p[1]+m[1]*length, p[2]+m[2]*length);
	glEnd(); 


	//glBegin(GL_LINES);	
	//glVertex3d(p[0], p[1], p[2]);
	//glVertex3f(p[0] - m[0]*length, p[1] - m[1]*length, p[2] - m[2]*length);
	//glEnd(); 

	//glEnable(GL_LIGHTING);
}

//wei begin
void GLDrawer::drawCamera(vcc::Camera& camera)
{
  //get the five control points of the cone
  Point3f far_end = camera.pos + camera.direction * camera.max_distance;
  Point3f top_right = far_end + camera.right * camera.horizon_dist / 2 
                        + camera.up * camera.vertical_dist / 2;
  Point3f top_left = far_end + camera.right * (-camera.horizon_dist) / 2
                        + camera.up * camera.vertical_dist / 2; 
  Point3f bottom_right = far_end + camera.right * camera.horizon_dist / 2
                        + camera.up * (-camera.vertical_dist / 2);
  Point3f bottom_left = far_end + camera.right * (-camera.horizon_dist / 2)
                        + camera.up * (-camera.vertical_dist / 2);

  glBegin(GL_LINES);
  glColor3f(1.0, 0.0, 0.0);
  glVertex(camera.pos); glVertex(top_left);
  glVertex(camera.pos); glVertex(top_right);
  glVertex(camera.pos); glVertex(bottom_left);
  glVertex(camera.pos); glVertex(bottom_right);
  glVertex(top_right); glVertex(top_left);
  glVertex(top_left); glVertex(bottom_left);
  glVertex(bottom_left); glVertex(bottom_right);
  glVertex(bottom_right); glVertex(top_right);
  glEnd();
}
//wei end

void GLDrawer::drawPickPoint(CMesh* samples, vector<int>& pickList, bool bShow_as_dot)
{
	double width = para->getDouble("Sample Draw Width");
	GLColor pick_color = para->getColor("Pick Point Color");
	glColor3f(pick_color.r, pick_color.g, pick_color.b);

	for(int ii = 0; ii < pickList.size(); ii++) 
	{
		int i = pickList[ii];

		if(i < 0 || i >= samples->vert.size())
			continue;

		CVertex &v = samples->vert[i];
		Point3f &p = v.P();     

		if(bShow_as_dot)
		{
			glPointSize(sample_dot_size * 1.2);
			glBegin(GL_POINTS);

			GLColor color = pick_color;
			glColor4f(color.r, color.g, color.b, 1);

			glVertex3d(p[0], p[1], p[2]);

			glEnd(); 
		}
		else
		{
			glDrawSphere(p, pick_color, sample_draw_width * 1.2, 40);
		}
	}    
}

void GLDrawer::glDrawLine(Point3f& p0, Point3f& p1, GLColor color, double width)
{
	glColor3f(color.r, color.g, color.b);
	glLineWidth(width);
	glBegin(GL_LINES);
	glVertex3f(p0[0], p0[1], p0[2]);
	glVertex3f(p1[0], p1[1], p1[2]);
	glEnd();
}

void RenderBone(float x0, float y0, float z0, float x1, float y1, float z1, double width = 20)  
{  
	GLdouble  dir_x = x1 - x0;  
	GLdouble  dir_y = y1 - y0;  
	GLdouble  dir_z = z1 - z0;  
	GLdouble  bone_length = sqrt( dir_x*dir_x + dir_y*dir_y + dir_z*dir_z );  
	static GLUquadricObj *  quad_obj = NULL;  
	if ( quad_obj == NULL )  
		quad_obj = gluNewQuadric();  
	gluQuadricDrawStyle( quad_obj, GLU_FILL );  
	gluQuadricNormals( quad_obj, GLU_SMOOTH );  
	glPushMatrix();  
	// 平移到起始点   
	glTranslated( x0, y0, z0 );  
	// 计算长度   
	double  length;  
	length = sqrt( dir_x*dir_x + dir_y*dir_y + dir_z*dir_z );  
	if ( length < 0.0001 ) {   
		dir_x = 0.0; dir_y = 0.0; dir_z = 1.0;  length = 1.0;  
	}  
	dir_x /= length;  dir_y /= length;  dir_z /= length;  
	GLdouble  up_x, up_y, up_z;  
	up_x = 0.0;  
	up_y = 1.0;  
	up_z = 0.0;  
	double  side_x, side_y, side_z;  
	side_x = up_y * dir_z - up_z * dir_y;  
	side_y = up_z * dir_x - up_x * dir_z;  
	side_z = up_x * dir_y - up_y * dir_x;  
	length = sqrt( side_x*side_x + side_y*side_y + side_z*side_z );  
	if ( length < 0.0001 ) {  
		side_x = 1.0; side_y = 0.0; side_z = 0.0;  length = 1.0;  
	}  
	side_x /= length;  side_y /= length;  side_z /= length;  
	up_x = dir_y * side_z - dir_z * side_y;  
	up_y = dir_z * side_x - dir_x * side_z;  
	up_z = dir_x * side_y - dir_y * side_x;  
	// 计算变换矩阵   
	GLdouble  m[16] = { side_x, side_y, side_z, 0.0,  
		up_x,   up_y,   up_z,   0.0,  
		dir_x,  dir_y,  dir_z,  0.0,  
		0.0,    0.0,    0.0,    1.0 };  
	glMultMatrixd( m );  
	// 圆柱体参数   
	GLdouble radius= width;        // 半径   
	GLdouble slices = 40.0;      //  段数   
	GLdouble stack = 3.0;       // 递归次数   
	gluCylinder( quad_obj, radius, radius, bone_length, slices, stack );   
	glPopMatrix();  
}  

void GLDrawer::glDrawCylinder(Point3f& p0, Point3f& p1, GLColor color, double width)
{
	glColor3f(color.r, color.g, color.b);
	RenderBone(p0[0], p0[1], p0[2], p1[0], p1[1], p1[2], width);
}


void GLDrawer::glDrawSphere(Point3f& p, GLColor color, double radius, int slide)
{
	if (radius < 0.0001)
		radius = 0.01;

	glColor3f(color.r, color.g, color.b);
	glPushMatrix();      
	glTranslatef(p[0], p[1], p[2]);
	glutSolidSphere(radius, slide, slide);
	glPopMatrix();
}

void GLDrawer::cleanPickPoint()
{
	curr_pick_indx = 0;
	prevPickIndex = 0;
}

void GLDrawer::glDrawBranches(vector<Branch>& branches, GLColor gl_color)
{
  if (useDifferBranchColor && random_color_list.size() < branches.size())
  {
    generateRandomColorList(branches.size() * 2);
  }

	for(int i = 0; i < branches.size(); i++)
	{
		Curve& curve = branches[i].curve;

		if (curve.empty())
		{
			cout << "curve empty!" << endl;
			continue;
		}

		if (curve.size() == 1)
		{
			Point3f p0 = curve[0];
			glDrawSphere(p0, GLColor(cBlue), skel_node_size, 40);

		}
		Point3f p0, p1;
		double size_scale = 0.5;
		//draw nodes
		for (int j = 0; j < curve.size(); j++)
		{
			Point3f p0 = p0 = curve[j].P();

			if (curve[j].is_skel_virtual)
			{
				glDrawSphere(p0, GLColor(feature_color), skel_node_size * 1.1, 40);
			}
      else if (useDifferBranchColor)
      {
        glDrawSphere(p0, GLColor(random_color_list[i]), skel_node_size, 40);
      }
			else
			{
				glDrawSphere(p0, GLColor(skel_node_color), skel_node_size, 40);
			}
		}

		for (int j = 0; j < curve.size()-1; j++)
		{
			p0 = curve[j].P();
			p1 = curve[j+1].P();

			if (skel_bone_width > 0.0003)
			{
        if (useDifferBranchColor)
        {
          glDrawCylinder(p0, p1, random_color_list[i], skel_bone_width);
        }
        else
        {
          glDrawCylinder(p0, p1, gl_color, skel_bone_width);
        }
			}
		}

	}
}

//void GLDrawer::glDrawCurves(vector<Curve>& curves, GLColor gl_color)
//{
//
//
//	for(int i = 0; i < curves.size(); i++)
//	{
//		Curve& curve = curves[i];
//
//		if (curve.empty())
//		{
//			cout << "curve empty!" << endl;
//			continue;
//		}
//
//		if (curve.size() == 1)
//		{
//			Point3f p0 = curve[0];
//			glDrawSphere(p0, GLColor(cBlue), skel_node_size, 40);
//
//		}
//		Point3f p0, p1;
//		double size_scale = 0.5;
//
//		//draw nodes
//		for (int j = 0; j < curve.size(); j++)
//		{
//			Point3f p0 = p0 = curve[j].P();
//
//			if (curve[j].is_skel_virtual)
//			{
//				glDrawSphere(p0, GLColor(feature_color), skel_node_size * 1.1, 40);
//			}
//      else if (useDifferBranchColor)
//      {
//        glDrawSphere(p0, GLColor(random_color_list[i]), skel_node_size, 40);
//      }
//			else
//			{
//				glDrawSphere(p0, GLColor(skel_node_color), skel_node_size, 40);
//			}
//		}
//
//		for (int j = 0; j < curve.size()-1; j++)
//		{
//			p0 = curve[j].P();
//			p1 = curve[j+1].P();
//
//			if (skel_bone_width > 0.0003)
//			{
//        if (useDifferBranchColor)
//        {
//          glDrawCylinder(p0, p1, random_color_list[i], skel_bone_width);
//        }
//        else
//        {
//          glDrawCylinder(p0, p1, gl_color, skel_bone_width);
//        }
//			}
//		}
//
//	}
//}


void GLDrawer::drawCurveSkeleton(Skeleton& skeleton)
{
	if (para->getBool("Skeleton Light"))
	{
		glEnable(GL_LIGHTING);
	}

	glDrawBranches(skeleton.branches, skel_bone_color);

	if (para->getBool("Skeleton Light"))
	{
		glDisable(GL_LIGHTING);
	}
}


void GLDrawer::generateRandomColorList(int num)
{
  if (num <= 1000)
    num = 1000;

  random_color_list.clear();

  srand(time(NULL));
  for(int i = 0; i < num; i++)
  {
    double r = (rand() % 1000) * 0.001;
    double g = (rand() % 1000) * 0.001;
    double b = (rand() % 1000) * 0.001;
    random_color_list.push_back(GLColor(r, g, b));
  }
}


void GLDrawer::drawSlice(Slice& slice, double trans_val)
{
  if (slice.slice_nodes.empty())
  {
    return;
  }
  Point3f p0, p1, p2, p3;

  //glPolygonMode(GL_SMOOTH);
  glShadeModel(GL_SMOOTH);   

  for (int i = 0; i < slice.res -1; i++)
  {
    for (int j = 0; j < slice.res -1; j++)
    {
      CVertex v0 = slice.slice_nodes[i * slice.res + j];
      CVertex v1 = slice.slice_nodes[i * slice.res + j + 1];
      CVertex v2 = slice.slice_nodes[(i+1) * slice.res + j + 1];
      CVertex v3 = slice.slice_nodes[(i+1) * slice.res + j];

      //GLColor c0 = getColorByType(v0);
      //GLColor c1 = getColorByType(v1);
      //GLColor c2 = getColorByType(v2);
      //GLColor c3 = getColorByType(v3);

      GLColor c0 = isoValue2color(v0.eigen_confidence, slice_color_scale, iso_value_shift, true);
      GLColor c1 = isoValue2color(v1.eigen_confidence, slice_color_scale, iso_value_shift, true);
      GLColor c2 = isoValue2color(v2.eigen_confidence, slice_color_scale, iso_value_shift, true);
      GLColor c3 = isoValue2color(v3.eigen_confidence, slice_color_scale, iso_value_shift, true);
      glBegin(GL_QUADS);
      glColor4f(c0.r, c0.g, c0.b, trans_val); glVertex3f(v0.P().X(), v0.P().Y(), v0.P().Z());
      glColor4f(c1.r, c1.g, c1.b, trans_val); glVertex3f(v1.P().X(), v1.P().Y(), v1.P().Z());
      glColor4f(c2.r, c2.g, c2.b, trans_val); glVertex3f(v2.P().X(), v2.P().Y(), v2.P().Z());
      glColor4f(c3.r, c3.g, c3.b, trans_val); glVertex3f(v3.P().X(), v3.P().Y(), v3.P().Z());
      glEnd();
    }
  }
}