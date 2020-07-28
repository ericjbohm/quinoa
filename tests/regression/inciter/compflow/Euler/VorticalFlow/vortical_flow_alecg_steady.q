# vim: filetype=sh:
# This is a comment
# Keywords are case-sensitive

title "Euler equations marching to steady state for vortical flow"

inciter

  ttyi 1
  cfl 0.5
  scheme alecg

  steady_state true
  residual 1.0e-8
  rescomp 1

  partitioning
   algorithm mj
  end

  compflow

    depvar c
    physics euler
    problem vortical_flow

    alpha 0.1
    beta 1.0
    p0 10.0

    material
      gamma 1.66666666666667 end
    end

    bc_dirichlet
      sideset 1 2 3 4 5 6 end
    end

  end

  plotvar
    interval 10
  end

  diagnostics
    interval  1
    format    scientific
    error l2
  end

end
