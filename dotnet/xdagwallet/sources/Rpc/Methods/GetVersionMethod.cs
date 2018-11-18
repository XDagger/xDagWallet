using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XDagNetWallet.Rpc.Methods
{
    public class GetVersionMethod : MethodBase
    {
        public GetVersionMethod(int identifier)
            : base(identifier, "xdag_version")
        {

        }



    }
}
