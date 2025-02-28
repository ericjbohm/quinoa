# vim: filetype=sh:
# This is a comment
# Keywords are case-sensitive

title "Advection of 2D Gaussian hump"

inciter

  nstep 100  # Max number of time steps
  dt   2.0e-3 # Time step size
  ttyi 1     # TTY output interval
  scheme dg

  transport
    physics advection
    problem gauss_hump
    ncomp 1
    depvar c

    bc_extrapolate
      sideset 1 end
    end
    bc_inlet
      sideset 2 end
    end
    bc_outlet
      sideset 3 end
    end
  end

  diagnostics
    interval  2
    format    scientific
    error l2
  end

  field_output
    interval 10
    var elem analytic C1 "c0_numerical" end
  end

end
