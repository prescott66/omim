package com.mapswithme.maps.editor;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;

import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.concurrency.ThreadPool;
import com.mapswithme.util.concurrency.UiThread;

public class OsmAuthFragment extends BaseAuthFragment implements View.OnClickListener
{
  protected static class AuthToolbarController extends ToolbarController
  {
    public AuthToolbarController(View root, Activity activity)
    {
      super(root, activity);
      mToolbar.setNavigationIcon(Graphics.tint(activity,
                                               activity.getResources().getDrawable(R.drawable.ic_cancel)));
    }

    @Override
    public void onUpClick()
    {
      super.onUpClick();
    }
  }

  private EditText mEtLogin;
  private EditText mEtPassword;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_osm_login, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mToolbarController.setTitle("Log In");
    mEtLogin = (EditText) view.findViewById(R.id.osm_username);
    mEtPassword = (EditText) view.findViewById(R.id.osm_password);
    view.findViewById(R.id.login).setOnClickListener(this);
    view.findViewById(R.id.lost_password).setOnClickListener(this);
  }

  @Override
  protected ToolbarController onCreateToolbarController(@NonNull View root)
  {
    return new AuthToolbarController(root, getActivity());
  }

  @Override
  public void onClick(View v)
  {
    // TODO show/hide spinners
    switch (v.getId())
    {
    case R.id.login:
      login();
      break;
    case R.id.lost_password:
      recoverPassword();
      break;
    }
  }

  private void login()
  {
    final String username = mEtLogin.getText().toString();
    final String password = mEtPassword.getText().toString();

    ThreadPool.getWorker().execute(new Runnable()
    {
      @Override
      public void run()
      {
        final String[] auth;
        auth = OsmOAuth.nativeAuthWithPassword(username, password);

        UiThread.run(new Runnable()
        {
          @Override
          public void run()
          {
            if (!isAdded())
              return;

            processAuth(auth);
          }
        });
      }
    });
  }

  private void recoverPassword()
  {
    startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.OSM_RECOVER_PASSWORD)));
  }
}
