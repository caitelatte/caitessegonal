// Tessegonal2D.cpp
// Copyright (c) 2014 Caitlin Macleod. All Rights Reserved.

static const char* const CLASS = "Tessegonal2D";
static const char* const HELP = 
	"Generates a tesselating image including octagons and squares.";

#include "DDImage/Iop.h"
#include "DDImage/Row.h"
#include "DDImage/Knobs.h"
#include "DDImage/Knob.h"
#include "DDImage/DDMath.h"

using namespace DD::Image;

namespace tessegonal2d
{

	//
	// Declarations
	//

	enum shape {
			SO1, // Seed Octagon 1
			SO2, // Seed Octagon 2
			DO1, // Dependent Octagon 1
			DO2, // Dependent Octagon 2
			FS1, // Filler Square 1
			FS2, // Filler Square 2
			OTH  // Other
		};

	class Tessegonal2D : public Iop {
		public:
			Tessegonal2D(Node* node);

			virtual const char* Class() const;
			virtual void knobs(Knob_Callback f);

			void _validate(bool);

			void engine(int y, int xx, int r, ChannelMask channels, Row& row);

			const char* displayName() const;
			const char* node_help() const;

			static const Description desc;

		private:
			float _base_radius;       // The main octagons' distance from center to any vertex when _transition_state == 0
			float _transition_state;  // The main octagons' scale compared to original. 0 <= _transition_state <= 1
			float _root_location [2]; // The original octagon's center point location. Sets the place where the pattern will seed from.
			float _colors [7] [4];    // RGBA values of different elements. Order is octagon1, octagon2, octagon3, octagon4, square1, square2
			FormatPair formats;
			int seedcenterx, seedcentery;
			int which_shape (int x, int y, bool row_a);

			// octagon helper functions
			bool inside_octagon (int x, int y, int octx, int octy, int octr);
				// checks if a given point (x, y) is inside an octagon with centre position octx, octy and radius octr
			bool inside_triangle (int px, int py, int t1x, int t1y, int t2x, int t2y, int t3x, int t3y);
				// checks if a given point (px, py) is inside a triangle with the given points.
			float sign (int p1x, int p1y, int p2x, int p2y, int p3x, int p3y);
				// takes the "perp-dot" of three points. used by inside_triangle.
			float oct_height (float radius);      // the distance from the centre of any edge to the centre of an octagon
			float oct_edge_length (float radius); // the length of any edge of an octagon
	}; // end class Tessegonal2D

	//
	// Implementations
	//

	Tessegonal2D(Node* node) : Iop(node)
		_base_radius(30.f);
		_transition_state(0.f);
	{
		inputs(0);

		_root_location[0] = _root_location[1] = 0.f;

		// init shape colors to default swatch
		// Seed Octagon 1
		_colors[SO1][0] = 178 / 255.0;
		_colors[SO1][1] =  25 / 255.0;
		_colors[SO1][2] =  30 / 255.0;

		// Seed Octagon 2
		_colors[SO2][0] = 213 / 255.0;
		_colors[SO2][1] =  74 / 255.0;
		_colors[SO2][2] =  48 / 255.0;

		// Dependent Octagon 1
		_colors[DO1][0] = 226 / 255.0;
		_colors[DO1][1] = 168 / 255.0;
		_colors[DO1][2] = 114 / 255.0;

		// Dependent Octagon 2
		_colors[DO2][0] = 163 / 255.0;
		_colors[DO2][1] =  96 / 255.0;
		_colors[DO2][2] =  67 / 255.0;

		// Filler Square 1
		_colors[FS1][0] = 243 / 255.0;
		_colors[FS1][1] = 236 / 255.0;
		_colors[FS1][2] = 224 / 255.0;

		// Filler Square 2
		_colors[FS2][0] = 243 / 255.0;
		_colors[FS2][1] = 236 / 255.0;
		_colors[FS2][2] = 224 / 255.0;

		// Other Color
		_colors[OTH][0] = _colors[OTH][1] = _colors[OTH][2] = 0 / 255.0;

		// alpha opaque
		_colors[SO1][3] = _colors[SO2][3] = _colors[DO1][3] = _colors[DO2][3] = _colors[FS1][3] = _colors[FS2][3] = _colors[OTH][3] = 255 / 255.0;

		// Default colours from a swatch within Adobe Illustrator's Textiles swatches.
	}

	const char* Tessegonal2D::Class() const { return CLASS; }

	void knobs(Knob_Callback f)
	{
		Format_knob(f, &formats, "format");
		Float_knob(f, &_base_radius, "base_radius", "Base Radius");
		Float_knob(f, &_transition_state, "transition_state", "Transition State");
		XY_knob(f, _root_location, "root_location", "Root Location");
		AColor_knob(f, _colors[0], "soct_1_color", "Seed Octagon 1");
		AColor_knob(f, _colors[1], "soct_2_color", "Seed Octagon 2");
		AColor_knob(f, _colors[2], "doct_1_color", "Dependent Octagon 1");
		AColor_knob(f, _colors[3], "doct_2_color", "Dependent Octagon 2");
		AColor_knob(f, _colors[4], "fsqr_1_color", "Dependent Square 1");
		AColor_knob(f, _colors[5], "fsqr_2_color", "Dependent Square 2");
		AColor_knob(f, _colors[6], "other_color",  "Other Color");
	}

	void _validate(bool)
	{
		info_.full_size_format(*formats.fullSizeFormat());
		info_.format(*formats.format());
		info_.channels(Mask_RGBA);
		info_.set(format());
	}

	void engine(int y, int xx, int r, ChannelMask channels, Row& row)
	{
		float* p[4];
		int shape_no = OTH;
		p[0] = row.writable(Chan_Red);
		p[1] = row.writable(Chan_Green);
		p[2] = row.writable(Chan_Blue);
		p[3] = row.writable(Chan_Alpha);

		// maths
		if (( (y % (2 * oct_height(_base_radius))) < (root_location[1] + oct_height(_base_radius) / 2) ) ||
			( (y % (2 * oct_height(_base_radius))) <= (root_location[1] - oct_height(_base_radius) / 2) ) )
			{ // if (y in row a)
				for (int x = xx; x < r; x++) 
				{ // for each x in row
					if ( (x % (2 * oct_height(_base_radius))) < (root_location[0] + oct_height(_base_radius) / 2) )
					{ // if (x in column 1)
						// store which shape
						shape_no = which_shape(x, y, true);
						// colour pixel appropriately
						for (int i = 0; i < 4; i++) {
							p[i][x] = _colors[shape_no][i];
						}
					} // end if in column
				} // end for each x
			} else { // else (row b)
				// if (x in column 2) {
					for (int x = xx; x < r; x++) 
					{ //for each x in row
						// store which shape
						shape_no = which_shape(x, y, false);
						// colour pixel appropriately
						for (int i = 0; i < 4; i++) {
							p[i][x] = _colors[shape_no][i]; 
						} 
					} // end for each x
				// } // end if in column
			} // end if in row
	}

	const char* displayName() const { return "Tessegonal2D"; }
	const char* node_help() const { return HELP; }


	static Iop* constructor(Node* node) { return new Tessegonal2D(node); }
	const Iop::Description Tessegonal2D::desc(CLASS, "Image/Tessegonal2D", constructor);

	int which_shape (int x, int y, bool row_a) {
		if row_a {
			return OTH;
		} else {
			return OTH;
		}
	}

	bool inside_octagon (int x, int y, int octx, int octy, int octr) {
		float hocth = oct_height(octr) / 2; // half the height of the octagon
		float hocte = oct_edge_length(octr) / 2; // half the length each of the octagon's edges
		float octl = octx - hocth; // left x coord
		float octr = octs + hocth; // right x coord
		float octt = octy + hocth; // top y coord
		float octb = octy - hocth; // bottom y coord
		float octli = octx + hocte; // left inner x coord
		float octri = octx - hocte; // right inner x coord
		float octti = octy + hocte; // top inner x coord
		float octbi = octy - hocte; // bottom inner x coord
		/* 
		// float oct[8][2];
		// oct[0][0] = octr;
		// oct[0][1] = octti;
		// oct[1][0] = octli;
		// oct[1][1] = octt;
		// oct[2][0] = octri;
		// oct[2][1] = octt;
		// oct[3][0] = octl;
		// oct[3][1] = octti;
		// oct[4][0] = octl;
		// oct[4][1] = octbi;
		// oct[5][0] = octli;
		// oct[5][1] = octb;
		// oct[6][0] = octri;
		// oct[6][1] = octb;
		// oct[7][0] = octr;
		// oct[7][1] = octbi; 
		*/
		if (( (x >= octl) && (x <= octr) ) && ( (x >= octb) && (x <= octt) )) { // inside the outer bounds
			if (x < octli) { // left
				if (y > octti) {
					return in_triangle (x, y, octl, octti, octli, octti, octli, octt);
				} else if (y < octbi) {
					return in_triangle (x, y, octl, octbi, octli, octbi, octli, octb);
				} else { // inside left rectangle
					return true;
				}
			} else if (x > octri) { // right 
				if (y > octti) {
					return in_triangle (x, y, octr, octti, octri, octti, octri, octt);
				} else if (y < octbi) {
					return in_triangle (x, y, octr, octbi, octri, octbi, octri, octb);
				} else { // inside right rectangle
					return true;
				}
			} else { // inside centre rectangle
				return true;
			}
		}
		return false;
	}

	/* perp-dot - basically the z-term in a  3D cross product.
	// based on code snippet and discussion by Kingnosis at 
	// http://www.gamedev.net/topic/295943-is-this-a-better-point-in-triangle-test-2d/ */
	float sign (int p1x, int p1y, int p2x, int p2y, int p3x, int p3y) {
		return ( (p1x - p3x) * (p2y - p2y) - (p2x - p3x) * (p1y - p3y) );
	}

	/* Tests if a given point is within a triangle defined by three points.
	// Based on a code snippet and discussion by kingnosis at 
	// http://www.gamedev.net/topic/295943-is-this-a-better-point-in-triangle-test-2d/
	*/
	bool in_triangle ( int px, int py, 
		int t1x, int t1y, int t2x, int t2y, int t3x, int t3y ) {
		bool b1, b2, b3;
		
		b1 = sign(px, py, t1x, t1y, t2x, t2y);
		b2 = sign(px, py, t2x, t2y, t3x, t2y);
		b3 = sign(py, py, t3x, t3y, t1x, t1y);

		return ( (b1 == b2) && (b2 == b3) );
	}

	float oct_height (float radius) { return 2 * 0.92388 * radius; }
	float oct_edge_length (float radius) { return 2 * 0.382683 * radius;}
} // end namespace tessegonal2d
