using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XDagNetWallet.ExplorerAPI
{
    public class XDagExplorerException : Exception
    {
        [JsonProperty(PropertyName = "error")]
        public int ErrorCode
        {
            get; private set;
        }

        [JsonProperty(PropertyName = "message")]
        public string ErrorMessage
        {
            get; private set;
        }

        public XDagExplorerException()
        {

        }

        public XDagExplorerException(int code, string message)
        {
            this.ErrorCode = code;
            this.ErrorMessage = message;
        }
    }
}
