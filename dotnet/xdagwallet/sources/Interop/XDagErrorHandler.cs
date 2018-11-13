using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XDagNetWallet.Interop
{
    public class XDagErrorHandler
    {
        public static XDagErrorCode ParseErrorCode(int errorCode)
        {
            if (Enum.IsDefined(typeof(XDagErrorCode), errorCode))
            {
                return (XDagErrorCode)errorCode;
            }

            return XDagErrorCode.Unknown;
        }
    }
}
