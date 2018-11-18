using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using XDagNetWallet.ExplorerAPI.Models;

namespace XDagNetWallet.ExplorerAPI
{
    public class XDagExplorerException : Exception
    {
        public string ErrorCode
        {
            get; set;
        }
        
        public string ErrorMessage
        {
            get; set;
        }

        public XDagExplorerException()
        {

        }

        public XDagExplorerException(string code, string message)
            : base(message)
        {
            this.ErrorCode = code;
            this.ErrorMessage = message;
        }

        public XDagExplorerException(ErrorData data)
            : base (data.ErrorMessage)
        {
            this.ErrorCode = data.ErrorCode;
            this.ErrorMessage = data.ErrorMessage;
        }
    }
}
