using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XDagNetWallet.Rpc
{
    public class JsonRpcException : Exception
    {
        public int ErrorCode
        {
            get; private set;
        }

        public string ErrorMessage
        {
            get; private set;
        }

        public JsonRpcException(int code, string message)
        {
            this.ErrorCode = code;
            this.ErrorMessage = message;
        }
    }
}
