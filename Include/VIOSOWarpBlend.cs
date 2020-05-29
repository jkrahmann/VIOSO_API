using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace VIOSOWarpBlend
{

    class Warper
    {
        public struct VWB_VEC3
        {
            public float x, y, z;
            public VWB_VEC3(float _x, float _y, float _z)
            {
                x = _x;
                y = _y;
                z = _z;
            }
        };
        public struct VWB_VEC4
        {
            public float x, y, z, w;
        };
        public struct VWB_MAT4X4
        {
            public float _11, _12, _13, _14;
            public float _21, _22, _23, _24;
            public float _31, _32, _33, _34;
            public float _41, _42, _43, _44;
        };
   

        public enum ERROR {
            NONE = 0,         /// No error, we succeeded
            GENERIC = -1,     /// a generic error, this might be anything, check log file
            PARAMETER = -2,   /// a parameter error, provided parameter are missing or inappropriate
            INI_LOAD = -3,    /// ini could notbe loaded
            BLEND = -4,       /// blend invalid or coud not be loaded to graphic hardware, check log file
            WARP = -5,        /// warp invalid or could not be loaded to graphic hardware, check log file
            SHADER = -6,      /// shader program failed to load, usually because of not supported hardware, check log file
            VWF_LOAD = -7,    /// mappings file broken or version mismatch
            VWF_FILE_NOT_FOUND = -8, /// cannot find mapping file
            NOT_IMPLEMENTED = -9,     /// Not implemented, this function is yet to come
            NETWORK = -10,        /// Network could not be initialized
            FALSE = -16,		/// No error, but nothing has been done
        };

        [DllImport("VIOSOWarpBlend.dll", EntryPoint = "VWB_CreateA", CallingConvention = CallingConvention.Cdecl)]
        public static extern int VWB_Create(IntPtr dxDevice, [MarshalAs(UnmanagedType.LPStr)]String szCnfigFile, [MarshalAs(UnmanagedType.LPStr)]String szChannelName, out IntPtr warper, Int32 logLevel, [MarshalAs(UnmanagedType.LPStr)]String szLogFile);
        [DllImport("VIOSOWarpBlend.dll", EntryPoint = "VWB_Init", CallingConvention = CallingConvention.Cdecl)]
        public static extern int VWB_Init(IntPtr warper);
        [DllImport("VIOSOWarpBlend.dll", EntryPoint = "VWB_Destroy", CallingConvention = CallingConvention.Cdecl)]
        public static extern int VWB_Destroy(IntPtr warper);
        //VWB_getViewProj, ( VWB_Warper* pWarper, VWB_float* pEye, VWB_float* pRot, VWB_float* pView, VWB_float* pProj));
        [DllImport("VIOSOWarpBlend.dll", EntryPoint = "VWB_getViewProj", CallingConvention = CallingConvention.Cdecl)]
        public static extern int VWB_getViewProj(IntPtr warper, ref VWB_VEC3 eye, ref VWB_VEC3 dir, ref VWB_MAT4X4 view, ref VWB_MAT4X4 proj );
        //VIOSOWARPBLEND_API( VWB_ERROR, VWB_render, ( VWB_Warper* pWarper, VWB_param src, VWB_uint stateMask ) );  
        [DllImport("VIOSOWarpBlend.dll", EntryPoint = "VWB_render", CallingConvention = CallingConvention.Cdecl)]
        public static extern int VWB_render(IntPtr warper, UInt32 stateMask );

        IntPtr _warper = Marshal.AllocHGlobal(sizeof(Int64));

        public Warper(Object dx, String iniFile, String channelName )
        {
            try
            {
                VWB_Create(dx, iniFile, channelName, out _warper, 2, "");
                if (null == _warper)
                    throw new System.FieldAccessException("Warper extension not found");
            }
            catch(Exception e)
            {
                _warper = IntPtr.Zero;
                throw new System.FieldAccessException("Warper extension not found");
            }
        }

        ~Warper()
        {
            if(null != _warper)
                VWB_Destroy(_warper);
        }

        public ERROR Init()
        {
            return (ERROR)VWB_Init(_warper);
        }

        public ERROR Render(UInt32 stateMask)
        {
            return (ERROR)VWB_render(_warper,stateMask);
        }

        public ERROR GetViewProj(ref VWB_MAT4X4 view, ref VWB_MAT4X4 proj)
        {
            VWB_VEC3 eye = new VWB_VEC3(0, 0, 0);
            VWB_VEC3 dir = new VWB_VEC3(0, 0, 0);

            return (ERROR)VWB_getViewProj(_warper,ref eye, ref dir, ref view, ref proj);
        }
    }

}
